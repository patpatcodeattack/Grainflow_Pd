#include <m_pd.h>
#include <algorithm>
#include <vector>
#include <array>
#include <gfPanner.h>
#include <atomic>
#include <forward_list>
#include <memory>


namespace
{
	constexpr int internal_block = 16;

	struct iolet
	{
		t_sample* vec;
		t_int nchans;
	};
}

static t_class* grainflow_stereoPan_tilde_class;

typedef struct _grainflow_stereoPan_tilde
{
	t_object x_obj;
	t_float f;
	t_inlet* main_inlet;
	t_inlet* second_inlet;
	t_outlet* main_outlet;
	t_outlet* list_outlet;


	std::array<std::vector<t_sample>, 2> output_channels;
	std::array<t_sample*, 2> output_channel_ptrs;
	std::array<t_sample**, 2> input_channel_ptrs;
	std::unique_ptr<Grainflow::gf_panner<internal_block, Grainflow::gf_pan_mode::stereo, t_sample>> panner;
	std::array<iolet, 2> input_data;
	std::array<iolet, 1> output_data;
	t_int blocksize{0};
	t_int samplerate{0};
	t_int channels{1};
	t_float pan_center{0.5f};
	t_float pan_spread{0.5f};

	t_clock* data_clock;
} t_grainflow_stereoPan_tilde;

void on_data_clock(int* w)
{
	auto x = reinterpret_cast<t_grainflow_stereoPan_tilde*>(w);
	if (x->panner == nullptr)
	{
		clock_delay(x->data_clock, 33);
		return;
	}
	auto positions = x->panner->get_positions();
	std::vector<t_atom> position_atoms;
	position_atoms.reserve(positions.size());
	for (auto const position : positions)
	{
		position_atoms.push_back(t_atom{A_FLOAT, t_word{position}});
	}

	outlet_list(x->list_outlet, &s_list, positions.size(), position_atoms.data());

	clock_delay(x->data_clock, 33);
}

void message_pan_center(_grainflow_stereoPan_tilde* x, t_float f)
{
	x->panner->pan_position = f;
}

void message_pan_spread(_grainflow_stereoPan_tilde* x, t_float f)
{
	x->panner->pan_spread = f;
}

void* grainflow_stereoPan_tilde_new(t_symbol* s, int ac, t_atom* av)
{
	s = nullptr;
	t_grainflow_stereoPan_tilde* x = reinterpret_cast<_grainflow_stereoPan_tilde*>(pd_new(
		grainflow_stereoPan_tilde_class));

	x->second_inlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	x->main_outlet = outlet_new(&x->x_obj, &s_signal);
	x->pan_center = 0.5f;
	x->pan_spread = 0.5f;


	if (ac > 0)
	{
		if (av[0].a_type != A_FLOAT) { return (void*)x; }
		x->pan_center = av[0].a_w.w_float;
	}
	if (ac > 1)
	{
		if (av[0].a_type != A_FLOAT) { return (void*)x; }
		x->pan_spread = av[1].a_w.w_float;
	}
	x->data_clock = clock_new(x, (t_method)on_data_clock);
	x->list_outlet = outlet_new(&x->x_obj, &s_list);
	clock_delay(x->data_clock, 33);

	return (void*)x;
}

t_int* grainflow_stereoPan_tilde_perform(t_int* w)
{
	auto x = reinterpret_cast<t_grainflow_stereoPan_tilde*>(w[1]);
	const int size = x->blocksize;
	for (int i = 0; i < x->input_channel_ptrs.size(); ++i)
	{
		for (int j = 0; j < x->channels; ++j)
		{
			x->input_channel_ptrs[i][j] = &(x->input_data[i].vec[size * j]);
		}
	}
	for (int i = 0; i < x->output_channels.size(); ++i)
	{
		x->output_channel_ptrs[i] = x->output_channels[i].data();
		std::fill_n(x->output_channels[i].data(), x->output_channels[i].size(), 0.0f);
	}

	x->panner->process(x->input_channel_ptrs[0], x->input_channel_ptrs[1],
	                   x->output_channel_ptrs.data(),
	                   size);


	for (int i = 0; i < x->output_data[0].nchans; ++i)
	{
		std::copy_n(x->output_channels[i].data(), size,
		            &(x->output_data[0].vec[i * size]));
	}

	return w + 2;
}

void grainflow_stereoPan_tilde_free(t_grainflow_stereoPan_tilde* x)
{
	for (auto& ch : x->input_channel_ptrs)
	{
		free(ch);
		ch = nullptr;
	}
	clock_free(x->data_clock);
}

static void grainflow_stereoPan_tilde_dsp(t_grainflow_stereoPan_tilde* x, t_signal** sp)
{
	x->channels = std::min(sp[0]->s_nchans, sp[1]->s_nchans);
	if (x->channels < 1) return;
	const auto out_start = 2;
	signal_setmultiout(&sp[out_start], 2); //Main inlet + additional ones
	for (auto& ch : x->output_channels)
	{
		ch.resize(sp[0]->s_length);
	}
	for (auto& ch : x->input_channel_ptrs)
	{
		free(ch);
		ch = static_cast<t_sample**>(malloc(sizeof(t_sample**) * x->channels));
	}

	for (int i = 0; i < x->input_data.size(); ++i)
	{
		x->input_data[i].nchans = sp[i]->s_nchans;
		x->input_data[i].vec = sp[i]->s_vec;
	}

	x->output_data[0].nchans = 2;
	x->output_data[0].vec = sp[2]->s_vec;
	x->blocksize = sp[0]->s_length;
	x->samplerate = sp[0]->s_sr;

	x->panner = std::make_unique<Grainflow::gf_panner<internal_block, Grainflow::gf_pan_mode::stereo, t_sample>>(
		x->channels, 2);
	x->panner->pan_position = x->pan_center;
	x->panner->pan_spread = x->pan_spread;
	dsp_add(grainflow_stereoPan_tilde_perform, 1, x);
}


extern "C" void setup_grainflow0x2estereoPan_tilde(void)
{
	grainflow_stereoPan_tilde_class = class_new(gensym("grainflow.stereoPan~"),
	                                            reinterpret_cast<t_newmethod>(grainflow_stereoPan_tilde_new),
	                                            reinterpret_cast<t_method>(grainflow_stereoPan_tilde_free),
	                                            sizeof(t_grainflow_stereoPan_tilde),
	                                            CLASS_MULTICHANNEL, A_GIMME, 0);
	class_addmethod(grainflow_stereoPan_tilde_class,
	                reinterpret_cast<t_method>(grainflow_stereoPan_tilde_dsp), gensym("dsp"), A_CANT, 0);
	CLASS_MAINSIGNALIN(grainflow_stereoPan_tilde_class, t_grainflow_stereoPan_tilde, f);
	class_addmethod(grainflow_stereoPan_tilde_class, reinterpret_cast<t_method>(message_pan_center),
	                gensym("panCenter"),
	                A_DEFFLOAT, 0);
	class_addmethod(grainflow_stereoPan_tilde_class, reinterpret_cast<t_method>(message_pan_spread),
	                gensym("panSpread"),
	                A_DEFFLOAT, 0);
}
