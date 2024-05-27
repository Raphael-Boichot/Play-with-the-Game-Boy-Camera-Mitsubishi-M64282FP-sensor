% Raphael BOICHOT 30/01/2023
% for any question : raphael.boichot@gmail.com

clear
clc
disp('-----------------------------------------------------')
disp('|Beware, this code is for Matlab ONLY !!!           |')
disp('|You can break the code with Ctrl+C on Editor Window|')
disp('-----------------------------------------------------')
% pkg load image
% pkg load instrument-control
arduinoObj = serialport("COM6",2000000); %set the Arduino com port here
%set(arduinoObj, 'timeout',-1);
flag=0;
image_counter=0;
x_tiles=0;
y_tiles=0;
title_reg='Empty';
mkdir ./image
while flag==0 %infinite loop
    data = readline(arduinoObj);
    if not(length(char(data))>100)
        disp(data);
    end
    if not(isempty(strfind(data,'x_tiles')));
        str=char(data);
        x_tiles=str2double(str(10:11));
    end
    if not(isempty(strfind(data,'y_tiles')));
        str=char(data);
        y_tiles=str2double(str(10:11));
    end
    if not(isempty(strfind(data,'Exposure')));
        str=char(data);
        title_reg=(str(end-4:end));
    end
    if strlength(data)>64 %This is an image coming
        data=(char(data));
        offset=1; %First byte is always junk (do not know why, probably a Println LF)
        im=[];
        %if length(data)>=16385
        for i=1:1:y_tiles*8 %We get the full image, 5 lines are junk at bottom, top is glitchy due to amplifier artifacts
            for j=1:1:x_tiles*8
                try
                    im(i,j)=hex2dec(data(offset:offset+1));
                catch
                    im(i,j)=im(i-1,j);
                end
                offset=offset+3;
            end
        end
        raw=im; %We keep the raw data just in case
        maximum=max(max(raw));
        minimum=min(min(raw));
        figure(1)
        subplot(1,2,1);
        histogram(raw,100)
        %end
        image_display=uint8(raw-minimum)*(255/(maximum-minimum));
        image_counter=image_counter+1;
        image_display=imresize(image_display,4,"nearest");
        imwrite(image_display,['./image/output_',num2str(image_counter),'.gif'],'gif');
        subplot(1,2,2);
        imshow(image_display)
        title(title_reg)
        drawnow
    end
end
