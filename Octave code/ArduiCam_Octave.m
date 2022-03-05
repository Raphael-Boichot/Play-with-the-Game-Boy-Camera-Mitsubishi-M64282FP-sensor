% Raphael BOICHOT 13/02/2022 
% for any question : raphael.boichot@gmail.com

clear
clc
disp('--------------------------------------------')
disp('|Beware, this code is for Octave ONLY !!!  |')
disp('--------------------------------------------')
pkg load image
pkg load instrument-control
arduinoObj = serialport("COM3",'baudrate',115200); %set the Arduino com port here
%configureTerminator(arduinoObj,"CR/LF");
flush(arduinoObj);
set(arduinoObj, 'timeout',-1);
flag=0;
num_image=1;

%N VH1 VH0 G4 G3 G2 G1 G0 
reg1=0b01100000;
%C17 C16 C15 C14 C13 C12 C11 C10 / exposure time by 4096 ms steps (max 1.0486 s)
reg2=0b00010000; %Use for indoor lighting
%C07 C06 C05 C04 C03 C02 C01 C00 / exposure time by 16 µs steps (max 4096 ms)
reg3=0b00000000; %Use for outdoor lighting
%P7 P6 P5 P4 P3 P2 P1 P0 
reg4=0b00000010; %filtering kernels
%M7 M6 M5 M4 M3 M2 M1 M0
reg5=0b00000001; %filtering kernels
%X7 X6 X5 X4 X3 X2 X1 X0
reg6=0b00000001; %filtering kernels
%E3 E2 E1 E0 I V2 V1 V0
reg7=0b00000111; %set Vref to 1 volts... because
%Z1 Z0 O5 O4 O3 O2 O1 O0 zero point calibration and output reference voltage
reg0=0b00111111; %set offset voltage to 1 volts to have 0-2 volts peak to peak voltages

while flag==0 %infinite loop

  register=[reg0 reg1 reg2 reg3 reg4 reg5 reg6 reg7];%free setting
  %register=[155 0 20 0 1 0 1 7];%Typical minimal settings (the image is smooth)
  %register=[128 96 100 0 2 5 1 7];%Typical register setting used by the GB camera with 2D edge enhancement
  
  for k=1:1:8
  fwrite(arduinoObj,char(register(k))); %send registers to Arduino
  end

data=[];
  while length(data)<1000  %flush serial
    data = ReadToTermination(arduinoObj);
    if not(isempty(strfind(data,'registers'))); %print the registers sent by Arduino for confirmation
      disp(data);
    end;
    
    if length(data)>1000 %This is an image coming
        offset=2; %First byte is kunk (do not know why)
        im=[];
        for i=1:1:128 %We get the full image, 5 lines are junk at bottom, top is glitchy due to amplifier artifacts
            for j=1:1:128
                im(i,j)=double(data(offset));
                offset=offset+1;
            end
        end
        raw=im; %We keep the raw data just in case
        im=im(9:end-8,:); %We get the intersting part of the image, what a Game Boy Camera displays for example
        subplot(1,2,1)
        imagesc(im)
        colormap gray
        subplot(1,2,2)
        hist(reshape(im,1,[]),255)
        drawnow
        %preparing for nice png output with autocontrast
        minimum=min(min(im));
        maximum=max(max(im));
        moyenne=mean(mean(im));
        disp(['mininum=',num2str(minimum),' maximum=',num2str(maximum), ' moyenne=',num2str(moyenne)])
        im=im-minimum;
        maximum=max(max(im));
        slope=255/maximum;
        im=im*slope;
        im=uint8(im);
        im=imresize(im,4,'nearest');
        imwrite(im,['Arduicam_',num2str(num_image),'.png'])
        %end nice png output with autocontrast
        disp(['Saving Arduicam_',num2str(num_image),'.png'])
        num_image=num_image+1;
    end
  end
end
