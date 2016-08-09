%Script to convert circular buffer PCM data output from CCS to a .wav file.

close all
clear;

FSI = 1.024e6; % digital mic sampling rate
M1 = 16; % CIC decimation factor
M2 = 2; % FIR1 decimation factor
M3 = 2; % FIR2 decimation factor
FS1 = FSI/M1; % FIR1 sampling rate
FS2 = FS1/M2; % FIR2 sampling rate
FSO = FS2/M3; % output sampling rate

fname = 'dig-mic-decimation_test_output.dat';
fname = strcat('..\test_data\output\', fname);

% Wav output file
wavout_fname = 'wavoutput_dig-mic-decimation_test.wav';

% Get digital mic output data

fid = fopen(fname, 'r');
if (fid == -1)
    error('Unable to open channel input file');
end
tline = fgets(fid);
in = fscanf(fid, '%d');
fclose(fid);

% Write to wav file 16kHz sampling rate
audiowrite(wavout_fname, in/2^15, FSO); 
