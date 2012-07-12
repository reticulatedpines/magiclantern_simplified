#!/usr/bin/env octave3.2

x = 1:360; y = 1:240;
x = [x, fliplr(x)];
y = [y, fliplr(y)];
[X,Y] = meshgrid(x,y);
XY = zeros(480,720);
for i = 1:480
	for j = 1:720
		if i/480 < (721-j)/720
			XY(i,j) = X(i,j) / 360;
		else
			XY(i,j) = Y(i,j) / 255;
		end
	end
end
imwrite(XY,'xy.tiff'); # defish this and save it as rectilin-xy.jpg
