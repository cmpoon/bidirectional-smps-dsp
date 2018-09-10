function [score, THD, P_fundamental, P_harmonics, P_total]= psd_score(data, avgT_samp, f_c, name)
n = length(data);
if mod(n, 2) == 1
    n = n - 1;
end
Y_FFT = fft(data,n);
P_FFT = Y_FFT.*conj(Y_FFT)/n;
f = (1/avgT_samp)/n*(0:floor((n-1)/2));

Ptrunc = sqrt(P_FFT(1:floor((n-1)/2+1)));
P_total = sum(Ptrunc);
P_harmonics = sum(Ptrunc(f >= (f_c+50)));
P_fundamental = sum(Ptrunc( ((f < (f_c+3)).*(f > (f_c-3)) > 0) ));
P_fundspread = P_total - P_fundamental - P_harmonics;

%THD = sqrt(P_harmonics) / P_fundamental;
THD = (P_harmonics) / P_fundamental;
score = THD + P_fundspread/P_fundamental;

figure
plot(f,Ptrunc);
grid on
grid minor
title(sprintf('%s Power Spectrum Density (THD = %g)', name, THD))
xlim([0 600])
xlabel('Frequency (Hz)')
ylabel(sprintf('%s Energy', name));

end