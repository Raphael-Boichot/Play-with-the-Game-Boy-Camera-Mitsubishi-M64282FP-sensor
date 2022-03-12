function [y]=Bayer_dithering(x,num,palette)
%palette=[0 90 180 255];
minus(1:palette(2))=palette(1);
minus(palette(2)+1:palette(3))=palette(2);
minus(palette(3)+1:palette(4)+1)=palette(3);
plus(1:palette(2))=palette(2);
plus(palette(2)+1:palette(3))=palette(3);
plus(palette(3)+1:palette(4)+1)=palette(4);
B2=[1 2; 3 0];
M=num*num;
B4=[4*B2+1 4*B2+2; 4*B2+3 4*B2+0];
B8=[4*B4+1 4*B4+2; 4*B4+3 4*B4+0];
B16=[4*B8+1 4*B8+2; 4*B8+3 4*B8+0];
B=[];
if num==2
    B=B2;
elseif num==4
    B=B4;
elseif num==8
    B=B8;
elseif num==16
    B=B16;
else
    disp('wrong value for bayer matrix');
end
T=round(255*(B+0.5)/M);
h=(112/num)-1;
w=(128/num)-1;
y=zeros(112,128);
for f=0:1:w
    for e=0:1:h
        for u=1:num
            for q=1:num
                r=(num*e)+u;
                d=(num*f)+q;
                if x(r,d)<=T(u,q);
                    y(r,d)=minus(x(r,d)+1);
                else
                    y(r,d)=plus(x(r,d)+1);
                end
            end
        end
    end
end