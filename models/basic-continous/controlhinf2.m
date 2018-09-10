clear
clc

close all
%%

L = 570e-6; 
R = 30e-3;

f=50;
w0=2*pi*f;

G=tf(1,[L,R]);
%GPWM=tf(1,[1/(20e3) 1])*tf(1,[1/(20e3) 1]);

Vmax=(70-40*sqrt(2))*1.5;
Imax=1700/40*sqrt(2);

Zbase=1700/40^2;
X=L*2*pi*50;
Xpu=X/Zbase;

G=G*Vmax/Imax;%*GPWM;

figure()
bode(G);
title('G');

%%

M=10^(2/20);
wB=2*pi*1e3;
A=1e-2

W1=tf([1/M wB],[1 wB*A]);


eta=0.02;
w0=2*pi*50;
W1N=tf([1 3.5e3],3.5e3)*tf([1 3.5e3],3.5e3);
k1=1/1e-2;
eta0=k1*0.5e-2/2;
W11=tf([k1*w0^2],[1 2*eta0*w0, w0^2]);
eta2=abs(k1)/abs(4*i*eta0-3)*1e-2;
W12=tf([1 2*(w0*2) (w0*2)^2],[1 2*eta2*(w0*2), (w0*2)^2]);
eta3=abs(k1)/(2*abs(3*i*eta0-4))*1e-2;
W13=tf([1 2*(w0*3) (w0*3)^2],[1 2*eta3*(w0*3), (w0*3)^2]);
W1M=W1N*W11*W12*W13;

bode(1/W1); hold on;
bode(1/W1M);
title('1/W1');

W1=W1M;

%%

W2=tf([1 2*pi*5e3],[2*pi*5e3])*tf([1 2*pi*5e3],[2*pi*5e3])*tf(1,[1/3e5 1])*tf(1,[1/3e5 1]);
figure();
bode(1/W2)
title('1/W2');

%%

W3=1/2*tf([1 2*pi*5e3],[2*pi*5e3]);
figure();
bode(1/W3);
title('1/W3');

%%

[K,CL,GAM,INFO]=mixsyn(G,W1,W2,[])

K=tf(K);
%%
figure();
T=feedback(series(G,K),1);
bode(T);
title('T');

figure();
S=feedback(1,series(G,K));
bode(S);
title('S');

figure('Position',[500,100,400,400]);
bode(W1*S); grid on; title('Shaping W1 S below 0 dB?');

%%

System = K; % Define System to reduce
Order = 6;
 
% Create option set for balred command
Options = balredOptions();
Options.FreqIntervals = [0 1.72e3]; % Frequency intervals to compute the limited gramians
 
% Compute reduced order approximation on specified frequency range
ReducedSystem = balred(System,Order,Options);

figure();
bode(K); hold on;
K=tf(ReducedSystem);
bode(ReducedSystem);

T=feedback(series(G,K),1);
S=feedback(1,series(G,K));


%%

ref=tf([w0 0],[1 0 w0^2])+tf([w0*2 0],[1 0 (w0*2)^2])/5+tf([w0*3 0],[1 0 (w0*3)^2])/7;
figure()
step(ref*T,0:1e-7:0.03)
hold on
step(ref,0:1e-7:0.03)
title('Ref tracking');

figure();
step(ref*K*S,0:1e-7:0.03);
title('Control action');

%%

Knotpu=K*Vmax/Imax;

denKinf=Knotpu.Denominator{1};
numKinf=Knotpu.Numerator{1};
%%
G=tf(1,[L,R]);
%GPWM=tf(1,[1/(20e3) 1])*tf(1,[1/(20e3) 1]);

%G=G*GPWM;

T=feedback(series(G,Knotpu),1);
S=feedback(1,series(G,Knotpu));

ref=Imax*tf([w0 0],[1 0 w0^2])+tf([w0*2 0],[1 0 (w0*2)^2])/5+tf([w0*3 0],[1 0 (w0*3)^2])/7;
figure()
step(ref*T,0:1e-7:0.03)
hold on
step(ref,0:1e-7:0.03)
title('Ref tracking');

figure();
step(ref*Knotpu*S,0:1e-7:0.03);
title('Control action');
