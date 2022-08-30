clc
clear
tic
data_short=load('Violent_exposure_change.csv');
toc
time=1
START=2;
SIN=3;
LOAD=4;
RESET=5;
CLOCK=6;
READ=7;
byte=[0 0 0 0 0 0 0 0 0 0 0];
reset=data_short(:,RESET);
list=find(reset==0);
clock_old=0;
registers=[];
list_registers=[];
counter=0;
for i=1:1:length(data_short)

    clock_new=data_short(i,CLOCK);% CLOCK
    if clock_old==0&&clock_new==1;
        byte=[byte, data_short(i,SIN)];% SIN
        byte=byte(2:end);
    end
    if clock_old==1&&clock_new==0&&data_short(i,LOAD)==1;% LOAD
        counter=counter+1;
        disp(byte);
        registers=[registers,byte(4:end)];
        if counter==8;
            counter=0;
        list_registers=[list_registers;registers];
        registers=[];
        end
    end

    clock_old=clock_new;

end
filename = 'Sensor_Registers.xlsx';
writematrix(list_registers,filename,'Sheet',1,'Range','M3')
