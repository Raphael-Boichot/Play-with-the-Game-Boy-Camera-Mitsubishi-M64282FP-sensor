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
arduinoObj = serialport("COM4",2000000); %set the Arduino com port here
%set(arduinoObj, 'timeout',-1);
flag=0;
image_counter=0;
mkdir ./image
while flag==0 %infinite loop
    data = readline(arduinoObj);
    data=(char(data));
    offset=1; %First byte is always junk (do not know why, probably a Println LF)
    im=[];
    if length(data)>=1000
        for i=1:1:32 %We get the full image, 5 lines are junk at bottom, top is glitchy due to amplifier artifacts
            for j=1:1:32
                im(i,j)=hex2dec(data(offset:offset+1));
                offset=offset+3;
            end
        end
        raw=im; %We keep the raw data just in case
        maximum=max(max(raw));
        minimum=min(min(raw));
        moyenne=mean(mean(raw));
        figure(1)
        subplot(1,2,1);
        histogram(raw,10)
    end
    image_display=uint8(raw-minimum)*(255/(maximum-minimum));
    image_counter=image_counter+1;
    image_display=imresize(image_display,20,"nearest");
    imwrite(image_display,['./image/output_',num2str(image_counter),'.gif'],'gif');
    subplot(1,2,2);
    imshow(image_display)
    drawnow
end

