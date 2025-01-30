local Wave = pd.Class:new():register("pr.wave")function Wave:initialize(sel, atoms)    if atoms[1] ~= nil then    self.n = atoms[1]    else    self.n = "nil"    end        self.grainNumber = 0    self.grainState = {}    self.grainProgress = {}    self.grainPosition = {}    self.grainAmp = {}    self.grainWindow = {}    self.grainChannel = {}    self.grainStream = {}    self.inlets = 1    self.outlets = 1    self.width = 400    self.height = 200    self.WaveformColor = {150,200,255}    self.GrainColor = {r = 50,g = 100, b = 255}    self.readPos = 0        --mouse info    self.mouseDown = false    self.mousePos = {0,0}    self.readline = false    -- waveform array    self.arrayAmount = 1    self.arrayLength = 1    self.startIndex = 0    self.arrayname = {self.n}    self.sizeH =  self.height /  self.arrayAmount    self.grainsize =  .1       --redraw speed   self.delay_time = 20*1   self.redawAble = false      self:set_size(self.width, self.height)   return trueendfunction Wave:postinitialize()  self.clock = pd.Clock:new():register(self, "tick")  self.clock:delay(self.delay_time)endfunction Wave:finalize()  self.clock:destruct()endfunction Wave:tick()    self:repaint(3)  self:repaint(2)  self.clock:delay(self.delay_time)endfunction Wave:mouse_down(x, y)    if self.mouseDown  and x >= 0 and y >= 0  and x <= self.width and y <= self.height then        self.readline = true        self.mouseDown = true        self.mousePos[1] = normalize(x,0,self.width)        self.mousePos[2] = normalize(y,self.height,0)        self.readPos = self.mousePos[1]        self:outlet(1,"list" , self.mousePos)        self:outlet(1,"list" , self.mousePos)    endendfunction Wave:mouse_drag(x, y)        if self.mouseDown  and x >= 0 and y >= 0  and x <= self.width and y <= self.height then        self.readline = true        self.mousePos[1] = normalize(x,0,self.width)        self.mousePos[2] = normalize(y,self.height,0)        self.readPos = self.mousePos[1]        self:outlet(1,"list" , self.mousePos)endendfunction Wave:mouse_up(x, y)     self.mouseDown = false      self.readline = falseend-- ====================================================function Wave:in_1_bang()   pd.post(tostring(self.mouseDown))  end    function Wave:in_1_buffername(atoms)        if atoms[1] ==  nil then                else            self.arrayAmount = # (atoms)            self.arrayname = atoms            self.sizeH =  self.height /  self.arrayAmount            self:repaint(2)        endendfunction Wave:in_1_grainState(atoms)    self.grainState = atoms           self.grainNumber = # (atoms)            endfunction Wave:in_1_grainProgress(atoms)    self.grainProgress = atoms    endfunction Wave:in_1_grainPosition(atoms)    self.grainPosition = atomsendfunction Wave:in_1_grainAmp(atoms)    self.grainAmp = atoms    self.redawAble = trueendfunction Wave:in_1_grainWindow(atoms)    self.grainWindow = atomsendfunction Wave:in_1_grainChannel(atoms)    self.grainChannel = atomsendfunction Wave:in_1_grainStream(atoms)    self.grainStream = atomsendfunction Wave:in_1_bufferList(atoms)         --   if #(atoms) ~= self.arrayAmount then            self.arrayAmount = # (atoms)            self.arrayname = atoms            self.sizeH =  self.height /  self.arrayAmount                  --      self:repaint(2)    --  endendfunction Wave:in_1_readPosition(atoms) --   self.readline = true --   self.readPos = atoms[1]  * self.width    endfunction Wave:in_1_recordHead(atoms)  self.readline = true    self.readPos = atoms[1]  * self.width    endfunction Wave:in_1_recordHeadSamps(atoms)        endfunction Wave:in_1_waveformColor(atoms)            self.WaveformColor = atoms             self:repaint(1)endfunction Wave:in_1_grainColor(atoms)            self.GrainColor.r = atoms[1]            self.GrainColor.g = atoms[2]            self.GrainColor.b = atoms[3]              --  self:repaint(3)endfunction Wave:paint(g)    g:set_color(100, 84, 108, 1)    g:stroke_rounded_rect(0, 0, self.width, self.height, 15, 2)  end-- ///////////////////////////////////function Wave:paint_layer_2(g)    local amount = self.arrayAmount    local sizeH =  self.height / amount    local wavehight = sizeH /2        for k = 1, amount do        local array = pd.table(self.arrayname[k])               if array then            local size = array:length()            local middleH = sizeH * (k-1) + (sizeH/2)            g:set_color(self.WaveformColor[1],self.WaveformColor[2],self.WaveformColor[3],1)            local chunck = math.floor(size / self.width)            local startx = 0            local starty = (array:get(0) * wavehight) + middleH            local W = Path(startx,starty )            for i = 1, self.width - 1 do               local val = array:get(chunck * i)               local x = i                local y =  (val * wavehight) +  middleH                W:line_to(x, y)             end            g:stroke_path(W, 1)        end            endend-- ======================================================function Wave:paint_layer_3(g)local channels = self.arrayAmount    if self.readline then    g:set_color(0,0,0,.5)    g:draw_line(self.readPos,0,self.readPos, self.height,2)    end        for i = 1, self.grainNumber  do            if self.grainState[i] == 1 and self.grainPosition[i]  and self.grainAmp[i] and self.grainChannel and self.grainProgress[i] then            local radius = ( self.sizeH * .1) *  (1 - 2 * (self.grainProgress[i] - 0.5)^2)        local x = self.grainPosition[i] * self.width        local y = (((self.grainChannel[i] - 1) * self.sizeH) + ((1 - self.grainAmp[i]) * self.sizeH) + ( self.sizeH * .1) + 4) * .8        local grainC = self.GrainColor                    g:set_color(grainC.r,grainC.g,grainC.b,.7)        g:fill_ellipse(x- (radius/2),y- (radius/2), radius,radius)        g:set_color(grainC.r,grainC.g,grainC.b,.9)        g:stroke_ellipse(x- (radius/2),y- (radius/2), radius,radius,1)        end   end   endfunction Wave:paint_layer_4(g)endfunction normalize(val,min,max)local normalized = (val- min)/(max- min)return normalizedend