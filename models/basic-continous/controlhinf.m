clear
clc

close all

L = 570e-6; 
R = 3e-3; 
f=50;

Ts=1/1e3;
Tdelay=(Ts*2);
Gdelay=tf([-Tdelay/2,1],[Tdelay/2, 1]);

G=tf(1,[L,R])*Gdelay;

Gvariation=tf(1,[1.05*L,3*R])*Gdelay;
Delta=(Gvariation-G)/G;
bode(Delta); hold on;
Gvariation=tf(1,[1.05*L,R])*Gdelay;
Delta=(Gvariation-G)/G;
bode(Delta); hold on;
Gvariation=tf(1,[0.95*L,3*R])*Gdelay;
Delta=(Gvariation-G)/G;
bode(Delta); hold on;
Gvariation=tf(1,[0.95*L,R])*Gdelay;
Delta=(Gvariation-G)/G;
bode(Delta); hold on;

%%
%W3=tf([1 2*pi*0.3e2],1)/(2*pi*0.3e2)*(0.8)*tf(1,[1/(2*pi*0.3e1) 1]);
W3=tf([1 2*pi*5e3],1)*(3e-5)*tf(1,[1/(2*pi*1e6) 1]);

bode(1/W3); hold on

%%

k1=20;
eta=0.02;
w0=2*pi*50;
W1N=tf([1 2e3],2e3)*tf([1 2e3],2e3);
W11=tf([k1*w0^2],[1 2*eta*w0, w0^2]);
W12=tf([1 2*(w0*2) (w0*2)^2],[1 2*eta*(w0*2), (w0*2)^2]);
W13=tf([1 2*(w0*3) (w0*3)^2],[1 2*eta*(w0*3), (w0*3)^2]);
W1=W11*W1N*W12*W13;

figure();
bode(1/W1); grid on; hold on;

%%

W2=tf([1/2e3 1],1)*tf([1/2e3 1],1)*(1/1e1)*tf(1,[1/5e4 1])*tf(1,[1/5e4 1]);
figure(1)
bode(1/W2); hold on

%%


[K,CL,GAM,INFO]=mixsyn(G,W1,W2,W3)

K=tf(K);

figure('Position',[1500,500,400,400]);
T=feedback(series(G,K),1);
bode(T); hold on;
bode(1/W3); grid on;

figure('Position',[0,100,400,400]);
S=feedback(1,series(G,K));
bode(S); hold on
bode(1/W1); grid on

figure('Position',[500,100,400,400]);
bode(W1*S); grid on; title('Shaping W1 S below 0 dB?');

figure('Position',[1000,100,400,400]);
bode(W3*T); grid on; title('Shaping W3 T below 0 dB?');

figure('Position',[1000,100,400,400]);
bode(W2*K*S); grid on; title('Shaping W2 KS below 0 dB?');

%%

ref=tf([w0 0],[1 0 w0^2])+tf([w0*2 0],[1 0 (w0*2)^2])/5+tf([w0*3 0],[1 0 (w0*3)^2])/7;
step(ref*T)
hold on
step(ref)