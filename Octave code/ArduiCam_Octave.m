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

while true

    data = ReadToTermination(arduinoObj);
    
    if length(data)>128*128+1
        offset=2;
        for i=1:1:128
            for j=1:1:128
                im(i,j)=double(data(offset));
                offset=offset+1;
            end
        end
        raw=im;
        im=im(9:end-8,:);
        imagesc(im)
        colormap gray
        minimum=min(min(im));
        maximum=max(max(im));
        title(['mininum=',num2str(minimum),' maximum=',num2str(maximum)])
        drawnow
        im=im-minimum;
        maximum=max(max(im));
        slope=255/maximum;
        im=im*slope;
        im=uint8(im);
        imresize(im,4,'nearest');
        imwrite(im,['Arduicam_',num2str(num_image),'.png'])
        disp(['Saving Arduicam_',num2str(num_image),'.png'])
        num_image=num_image+1;
    end
end
