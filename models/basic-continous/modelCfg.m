%% 
% Bidirectionl Power Converter Sim
% Simulink Model Configuration
close all
%clear all

modelFile = 'Rev14_DCtoAC_Tuning';
%CtrlParams = [0.02 1.05 1 1 1 1 3 4 2 0 -30];

%% Controller Parameters
if exist('CtrlParams','var') == 1 && length(CtrlParams) == 11
	fprintf("\r\n === Starting Revision 12 Simulation ===\n");
	
   VCtrl_new = CtrlParams(1:4);
   ICtrl_new = CtrlParams(5:10);
   floor_dB  = CtrlParams(11);
   disp("Applying Controller Cfg: (See below for breakdown)");
   disp(CtrlParams);
end

%% World parameters
f = 50; % 50Hz AC supply
w_e=2*pi*f; % omega_elec
Vac = 40; % RMS Vgrid AC supply
Vacpp = Vac .* sqrt(2); % Peak to peak Vgrid AC supply

% Specification parameters
Vdc = 70; % Regulated DC Output
Pmax = 1700; % Max Power flow
f_s = 20e3; % Vmosfet Switching frequency

% Calc power base for pu and max values
Idcmax = Pmax / Vdc;
Iacmax=Pmax/Vac;
Zbase=Vac/Iacmax;
T = 1/f_s; % switching time period

%% Component Values
% Inductor AC Filter Est
% Rule of thumb X = 0.2 pu.
% Too high and higher voltage is required on the converter side. Too low and filtering is bad.
Xfilter=0.2*Zbase;  
Lfilter=Xfilter/w_e;

% PCB Soldered Components
% Inductor
L = 570e-6; % 570uH on 
R = 3e-3; % 3mH: Value of inductor parastitic resistance

C = 50e-3;  % 5 x 10mF
Rcap = 0.06/7; % this is more or less the resistance one gets with 14 capacitors SLPX223M050H9P3 combined


%% Voltage controller
% Use empirial rule
%
% VCtrlTune:
% +-- maxDelta --¬ +-- PI K Gain-¬
%  %VdcSP  %IdcMax     %Kp    %Ki
VCtrlTune = [0.05   1.47  1    1];

if exist('VCtrl_new','var') && length(VCtrl_new) == 4
    VCtrlTune = VCtrl_new;
end

maxDeltaVdc = VCtrlTune(1) .* Vdc;
maxDeltaIdc = VCtrlTune(2) .* Idcmax; % 24.3 amps at 1.7kw 70v 

Kp_v = VCtrlTune(3) .* -2 .* maxDeltaIdc ./ (exp(1) .* maxDeltaVdc);
Ki_v = VCtrlTune(4) .* -1 .* (Kp_v .^ 2)/ (4.*C);

H_VCtrl = tf([Kp_v Ki_v],[1 0]);

t_peak = -2 .* C/Kp_v;

disp('VCtrl Parameters:');
disp('+------ maxDelta -----¬ +------ PI K------¬');
disp('   %VdcSP     %IdcMax     Kp     Ki     t_pk');
disp([VCtrlTune(1) VCtrlTune(2) Kp_v Ki_v t_peak ]);

%% Current Controller
% Current controller configs
% Based on rule of thumb

% ICtrl Gain Coefficients
% Kp Kpr_base Kpr1 Kpr3 Kpr5 Kpr7
% Fail at 2.63s neg ramp up: 1 2 6 5 
% 1 1 2 4 2: OK
% 1 1 3 4 2: OK
ICtrl_coeff = [1.9 1 1.43 0.91 1.87 0.34];
if exist('ICtrl_new','var') && length(ICtrl_new) == 6
    ICtrl_coeff = ICtrl_new;
end

%  Tau* ~ 1ms for Iout settling
tau = 2e-3; % 10% of cycle
assert (tau < L/R)

disp(['ICtrl Tuning Parameters: ']);
disp('Kp    Kpr_base   Kpr1    Kpr3   Kpr5    Kpr7');
disp(ICtrl_coeff);

% Calc paramters
Kp_i = L./tau .* ICtrl_coeff(1);
Ki_i = R./tau .* ICtrl_coeff(2);

Kr_i1 = Ki_i .* ICtrl_coeff(3);
Kr_i2 = Ki_i .* ICtrl_coeff(4);
Kr_i3 = Ki_i .* ICtrl_coeff(5);
Kr_i4 = Ki_i .* ICtrl_coeff(6);

% Generate tfs
kp = tf(Kp_i);

kr1 = tf([Kr_i1 0], [1 0 (2*pi*f)^2]);
kr2 = tf([Kr_i2 0], [1 0 (2*pi*f*3)^2]);
kr3 = tf([Kr_i3 0], [1 0 (2*pi*f*5)^2]);
kr4 = tf([Kr_i4 0], [1 0 (2*pi*f*7)^2]);


H_pr = kr1+kr2+kr3+kp;



%% Notch filter
% To remove 100hz sensor hum
w_n = 2 * w_e;  % Centre freq 100Hz
bw = 4 * 2 *pi; % 2Hz bandwidth
if exist('floor_dB','var') ~= 1
floor_dB = -80;
end

floorcoeff = 10^(floor_dB/10+ 8/10)*bw;
disp(['Floor dB    => s Coefficient']);
disp([floor_dB floorcoeff]);
H_notch_2w = tf([1 floorcoeff (w_n^2)],[1 2*bw w_n^2]);
H_notch_1w = tf([1 floorcoeff (w_e^2)],[1 2*bw w_e^2]);
H_notch_combined = series(H_notch_2w, H_notch_1w);
H_VCtrlFiltered = series(H_notch_combined, H_VCtrl);
    
if ~exist('autoSim','var')

    figure
    bode(H_pr)
    grid minor
    title("Proportional Resonantor")

    figure
    bode(H_notch_combined)
    grid minor
    title("Implemented Notch Filter 100Hz & 50Hz")
    

    figure
    bode(H_VCtrlFiltered)
    grid minor
    title("Voltage Controller with Filter")
end 

%% Input RC Prefilter

RinpFtr = 15e3;
CinpFtr = 1e-9;



H_inpFtr = tf([1], [R.*C 1]);
figure
bode(H_inpFtr)
grid minor
title("10kHz Inputs Prefilter")


%% Go!
minSimLength = 4.5;

sessionName = sprintf("%s_%s",mat2str(VCtrlTune),mat2str(ICtrl_coeff));
sessionWSFile = sprintf('sessionData/tmp-Rev12-runningSimCfg-%s.mat', sessionName);
save(sessionWSFile);
evalin('base', sprintf('load(''%s'')',sessionWSFile));
fprintf("\r> Sim Starting < [%s] \n",datestr(now,'yyyy-mm-dd HH:MM:SS'));
%open scope to monitor progress
tic
%open_system(sprintf('%s/MasterTestbenchScope', modelFile));
SimOut = sim(modelFile,...
    'SaveOutput','on','OutputSaveName','simStabilityNew',...
    'IgnoredZcDiagnostic','none');
simRuntime = toc;

delete(char(sessionWSFile));
simStability = SimOut.simStabilityNew;

VdcOut = simStability{1}.Values.Data;
IacOut = simStability{2}.Values.Data;

simLength = max(SimOut.tout);
samples = length(VdcOut);
avgT_samp = simLength/samples;

if avgT_samp > T
   warning('Average sample time is greater than device switching period! Results may not be reliable. \r\n > T_samp=%g; T_sw=%g ==> Factor; %g', avgT_samp, T, avgT_samp/T);
end


unstable = VdcOut(end) < 55 || simLength < minSimLength;
stability = "STABLE :)";
if unstable, stability = "UNSTABLE!"; end

VdcExcursionScore = sum(abs(VdcOut - 70)/maxDeltaVdc)/samples;

fprintf("=== SIM END ===\n>> %s\r\nRuntime: %g; Sim period: %g; Samples: %i; Samp Rate: %g\n", stability, simRuntime, simLength, samples, 1/avgT_samp);

disp('Vout Regulation:');
disp('   End      Avg       Max       Min      Score');
disp([VdcOut(end) mean(VdcOut)...
    max(VdcOut) min(VdcOut) VdcExcursionScore]);


[IacHarmonicScore, THD_Iac, P_fund_Iac, P_harm_Iac, P_tot_Iac]= psd_score(IacOut, avgT_samp, 50, "I_{AC}");
[VdcHarmonicScore, THD_Vdc, P_fund_Vdc, P_harm_Vdc, P_tot_Vdc]= psd_score(VdcOut, avgT_samp, 0, "V_{DC}");

totalPenalty = 10*VdcExcursionScore + 1*IacHarmonicScore + 5*VdcHarmonicScore + unstable*500;

sessionSaveFile = 'sessionData/Rev12_runSummary.mat';
if exist(sessionSaveFile,'file') == 2
    load(sessionSaveFile);
else
    rev12_sessionData = {};
end

rev12_sessionData = [rev12_sessionData;...
    {[VCtrlTune ICtrl_coeff floor_dB]}...
    {[totalPenalty VdcExcursionScore IacHarmonicScore VdcHarmonicScore unstable]}];

save(sessionSaveFile, 'rev12_sessionData');

fprintf("\rSim Results Summary\r");
disp(rev12_sessionData{end});

loggerHdl = Simulink.sdi.createRun(SimOut);

if ~unstable
    %save
    clear SimOut
    clear simStability
    save(sprintf('sessionData/Rev12_run-s_%g.mat',totalPenalty));
end

if exist('CtrlParams','var') == 1 && length(CtrlParams) == 11 && ...
        exist('autoSim','var') && autoSim
   %discard sim results to prevent mem leak
   Simulink.sdi.deleteRun(loggerHdl);
   Simulink.sdi.clear
   clearvars -except totalPenalty
end
    

fprintf("----- END -----\r\n");