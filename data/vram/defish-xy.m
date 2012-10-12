#!/usr/bin/env octave3.2

x = linspace(0,360,1440); y = linspace(0,240,960);
x = [x, fliplr(x)];
y = [y, fliplr(y)];
[X,Y] = meshgrid(x,y);
size(X)
XY = zeros(480*4,720*4);
size(XY)
for i = 1:480*4
	for j = 1:720*4
		if i/480/4 < (721-j/4)/720
			XY(i,j) = X(i,j) / 360;
		else
			XY(i,j) = Y(i,j) / 255;
		end
	end
end
imwrite(XY,'xy.tiff'); # defish this and save it as my-lut-filename.tif
