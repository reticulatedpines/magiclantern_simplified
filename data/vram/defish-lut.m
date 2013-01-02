#!/usr/bin/env octave3.2

prefix = argv(){1};

xy = double(imread([prefix ".tif"]));

f = fopen([prefix ".lut"], "wb")
for i = 1:480/2
	for j = 1:720/2
		fwrite(f, [xy(i,j), xy(481-i,721-j)], 'uint8');
	end
end
fclose(f)
