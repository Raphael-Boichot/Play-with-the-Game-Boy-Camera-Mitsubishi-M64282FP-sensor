% Raphael BOICHOT 13/02/2022
% for any question : raphael.boichot@gmail.com

clear
clc
autoexposure='ON'%or OFF
disp('-----------------------------------------------------')
disp('|Beware, this code is for Matlab ONLY !!!           |')
disp('|You can break the code with Ctrl+C on Editor Window|')
disp('-----------------------------------------------------')
% pkg load image
% pkg load instrument-control
arduinoObj = serialport("COM3",2000000); %set the Arduino com port here
%set(arduinoObj, 'timeout',-1);
flag=0;
num_image=1;
error=imread('Error.png');
mkdir('./images/')

reg1=0b01100000;%N VH1 VH0 G4 G3 G2 G1 G0
reg2=0b00000001;%C17 C16 C15 C14 C13 C12 C11 C10 / exposure time by 4096 ms steps (max 1.0486 s)
reg3=0b00000000;%C07 C06 C05 C04 C03 C02 C01 C00 / exposure time by 16 µs steps (max 4096 ms)
reg4=0b00000010;%P7 P6 P5 P4 P3 P2 P1 P0 filtering kernels
reg5=0b00000001;%M7 M6 M5 M4 M3 M2 M1 M0 filtering kernels
reg6=0b00000001;%X7 X6 X5 X4 X3 X2 X1 X0 filtering kernels
reg7=0b00000011;%E3 E2 E1 E0 I V2 V1 V0 set Vref
reg0=0b10100000;%Z1 Z0 O5 O4 O3 O2 O1 O0 zero point calibration and output offset voltage


while flag==0 %infinite loop
    register=[reg0 reg1 reg2 reg3 reg4 reg5 reg6 reg7];%free setting
    reg2=double(reg2);
    reg3=double(reg3);
    %register=[155 0 1 0 1 0 1 7];%Typical minimal settings (the image is smooth)
    %register=[128 96 1 0 2 5 1 7];%Typical register setting used by the GB camera with 2D edge enhancement
    data=[];
    data = readline(arduinoObj);
    if not(strlength(data)>100)
        disp(data);
    end

    if not(isempty(strfind(data,'Ready'))); %Inject the registers
        for k=1:1:8
            write(arduinoObj,register(k),'uint8'); %send registers to Arduino
        end
    end;
    if strlength(data)>1000 %This is an image coming
        data=double(char(data));
        offset=1; %First byte is always junk (do not know why, probably a Println LF)
        im=[];
        if length(data)>=16385
            for i=1:1:128 %We get the full image, 5 lines are junk at bottom, top is glitchy due to amplifier artifacts
                for j=1:1:128
                    im(i,j)=double(data(offset));
                    offset=offset+1;
                end
            end
            raw=im; %We keep the raw data just in case
            im=im(9:end-8,:); %We get the intersting part of the image, what a Game Boy Camera displays for example
        end
        
        if length(data)<16385 % not enough data whatever the reason
            im=error(:,:,1);
        end

        %preparing for nice png output with autocontrast
        minimum=min(min(im));
        maximum=max(max(im));
        moyenne=mean(mean(im));
        disp(['mininum=',num2str(minimum),' maximum=',num2str(maximum), ' moyenne=',num2str(moyenne)])
        disp(['Number of unique pixel value: ',num2str(length(unique(reshape(im,1,[]))))])
        im=im-minimum;
        maximum=max(max(im));
        slope=255/maximum;
        im=im*slope;
        im=uint8(im);
        
        bayer=Bayer_dithering(im,8,[0 84 168 255]);%second argument is the order
        subplot(2,2,1)
        imagesc(im)
        title('Live view 128x112')
        subplot(2,2,2)
        imagesc(raw)
        title('Raw 128x128 image')
        colormap(gray)
        title('Dithered image')
        subplot(2,2,3)
        imagesc(bayer)
        subplot(2,2,4)        
        hist(data,50)
        title('Pixel histogramm')
        colormap(gray)
        drawnow
        
        
        im=imresize(im,4,'nearest');
        imwrite(im,['./images/Arduicam_',num2str(num_image),'.png'])
        %end nice png output with autocontrast
        disp(['Saving Arduicam_',num2str(num_image),'.png'])
        num_image=num_image+1;
        
        if autoexposure=='ON'
            delta=moyenne-70;
            disp(['Autoexposure delta = ',num2str(delta)])
            disp(['Autoexposure rg2 = ',num2str(reg2)])
            disp(['Autoexposure rg3 = ',num2str(reg3)])
            disp(['Black pixels level = ',num2str(mean(mean(raw(end-4:end,:))))])
            %disp(['Exposure time = ',num2str(reg3*16e-6+reg2*255*16e-6),' seconds'])
            
            if not(reg2==0); reg3=reg3-min(20*round(delta),255);end;
            if reg2==0; reg3=reg3-round(0.2*delta);end;
            
            if reg3>255;
                reg3=reg3-255;
                reg2=reg2+1;
            end;
            if reg3<0;
                reg3=reg3+255;
                reg2=reg2-1;
            end;
            if reg2<0;reg2=0;end;
            if reg3<0;reg3=0;end;
            if reg2>255;reg2=255;end;
            if reg3>255;reg3=255;end;
            if reg3==0&&reg2==0;reg3=1;end;%minimal exposure time

        end
    end
end
