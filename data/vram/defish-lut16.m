#!/usr/bin/env octave3.2

prefix = argv(){1};

xy = double(imread([prefix ".tif"]));

% smooth the image a bit to fake 16-bit data
x = -4:4; y = -4:4;
[x,y] = meshgrid(x,y);
r = sqrt(x.^2 + y.^2);

%~ % try to find an optimum smoothing factor that preserves a straight line
%~ E = [];
%~ F = linspace(1,10,100);
%~ for f = F,
	%~ %B = sinc(r/f);
	%~ B = cos(r/f); 
	%~ B(B<0) = 0; B /= sum(B(:)); 
	%~ xyf = filter2(B, xy(1:150,1:150)); 
	%~ 
	%~ yp = xyf(100, 10:100); xp = 1:length(yp);
	%~ a = polyfit(xp,yp,1);
	%~ yps = a(1)*xp + a(2);
	%~ e = norm(yp-yps);
	%~ E(end+1) = e;
%~ end
%~ plot(E);
%~ [m,i] = min(E);
%~ [F(i), m]

B = cos(r/3.3); 
B(B<0) = 0; B /= sum(B(:)); 
xyf = filter2(B, xy); 

%~ B = ones(7); B /= sum(B(:));
%~ xyf = filter2(B, xy);
%~ figure(2), plot(xyf(100, 10:50))

xyf = xyf(1:4:end, 1:4:end); % resize to 720x480
%~ size(xyf)

%~ plot(xy(240,:)), hold on, plot(xyf(240,:),'r')

f = fopen([prefix ".lut"], "wb")
for i = 1:480/2
	for j = 1:720/2
		fwrite(f, [xyf(i,j), xyf(481-i,721-j)] * 256, 'uint16');
	end
end
fclose(f)
