% -----------------------------------------------------------------
% Cuckoo Search (CS) algorithm by Xin-She Yang and Suash Deb      %
% Programmed by Xin-She Yang at Cambridge University              %
% -----------------------------------------------------------------
% Papers -- Citation Details:
% 1) http://arxiv.org/PS_cache/arxiv/pdf/1003/1003.1594v1.pdf 
% 2) http://arxiv.org/PS_cache/arxiv/pdf/1005/1005.2908v2.pdf
% =============================================================== %
% Notes:                                                          %
% Different implementations may lead to slightly different        %
% behavour and/or results, but there is nothing wrong with it,    %
% as this is the nature of random walks and all metaheuristics.   %
% -----------------------------------------------------------------


function [bestnest,fmin]=cuckoo_search_new(n)
if nargin<1
% Number of nests (different solutions)
n=5; %25 seems too much
end

% Discovery rate of alien eggs/solutions
pa=0.25;

%% Change this if you want to get better results
N_IterTotal=100;
%% Simple bounds of the search domain
% Lower bounds
nd=10; 
Lb=[0.01 0.8 1 1 1.0 1 1 1 0 0 -10]; 
% Upper bounds
Ub=[0.05 1.5 1 1 1.5 1 5 5 6 3 -60];

% Random initial solutions
for i=1:n
nest(i,:)=Lb+(Ub-Lb).*rand(size(Lb));
end

resumeFile = 'cs_resume.mat';

if ~exist(resumeFile,'file') == 2
    % Get the current best
    fitness=10^10*ones(n,1);
    [fmin,bestnest,nest,fitness]=get_best_nest(nest,nest,fitness);
    fprintf("## New Parameter Search ##");
    N_iter=0;
    %% Starting iterations
    iter = 1;
else
    load(resumeFile);
    fprintf(">> Resuming from iteration %i of %i\r\n", iter, N_IterTotal);
    warning("Resuming CS will not observe current bounds or guarantee randomness entropy!");
    warning("Using number of nests %i with the following bounds:", n);
    disp(Ub);
    disp(Lb);
end

while iter<N_IterTotal,
 
    % Generate new solutions (but keep the current best)
     new_nest=get_cuckoos(nest,bestnest,Lb,Ub);   
     [fnew,best,nest,fitness]=get_best_nest(nest,new_nest,fitness);
    % Update the counter
      N_iter=N_iter+n; 
    % Discovery and randomization
      new_nest=empty_nests(nest,Lb,Ub,pa) ;
    
    % Evaluate this set of solutions
      [fnew,best,nest,fitness]=get_best_nest(nest,new_nest,fitness);
    % Update the counter again
      N_iter=N_iter+n;
    % Find the best objective so far  
    if fnew<fmin,
        fmin=fnew;
        bestnest=best;
    end
    
    fprintf("\n###########\r\n>> Iteration: %i of %i done! \r\n###########\n>  CurrentBest:\n", iter, N_IterTotal); 
    disp(fnew);
    fprintf("... from nest ...\n");
    disp(best);
    
    iter=iter+1;
    save(resumeFile);
end %% End of iterations

%% Post-optimization processing
%% Display all the nests
disp(strcat('Total number of iterations=',num2str(N_iter)));
fmin
bestnest
save('cs_results.mat')

%% --------------- All subfunctions are list below ------------------
%% Get cuckoos by ramdom walk
function nest=get_cuckoos(nest,best,Lb,Ub)
% Levy flights
n=size(nest,1);
% Levy exponent and coefficient
% For details, see equation (2.21), Page 16 (chapter 2) of the book
% X. S. Yang, Nature-Inspired Metaheuristic Algorithms, 2nd Edition, Luniver Press, (2010).
beta=3/2;
sigma=(gamma(1+beta)*sin(pi*beta/2)/(gamma((1+beta)/2)*beta*2^((beta-1)/2)))^(1/beta);

for j=1:n
    s=nest(j,:);
    % This is a simple way of implementing Levy flights
    % For standard random walks, use step=1;
    %% Levy flights by Mantegna's algorithm
    u=randn(size(s))*sigma;
    v=randn(size(s));
    step=u./abs(v).^(1/beta);
  
    % In the next equation, the difference factor (s-best) means that 
    % when the solution is the best solution, it remains unchanged.     
    stepsize=0.01*step.*(s-best);
    % Here the factor 0.01 comes from the fact that L/100 should the typical
    % step size of walks/flights where L is the typical lenghtscale; 
    % otherwise, Levy flights may become too aggresive/efficient, 
    % which makes new solutions (even) jump out side of the design domain 
    % (and thus wasting evaluations).
    % Now the actual random walks or flights
    s=s+stepsize.*randn(size(s));
   % Apply simple bounds/limits
   nest(j,:)=simplebounds(s,Lb,Ub);
end

%% Find the current best nest
function [fmin,best,nest,fitness]=get_best_nest(nest,newnest,fitness)
% Evaluating all new solutions
for j=1:size(nest,1),
    fnew=evalModel(newnest(j,:));
    if fnew<=fitness(j),
       fitness(j)=fnew;
       nest(j,:)=newnest(j,:);
    end
end
% Find the current best
[fmin,K]=min(fitness) ;
best=nest(K,:);

%% Replace some nests by constructing new solutions/nests
function new_nest=empty_nests(nest,Lb,Ub,pa)
% A fraction of worse nests are discovered with a probability pa
n=size(nest,1);
% Discovered or not -- a status vector
K=rand(size(nest))>pa;

% In the real world, if a cuckoo's egg is very similar to a host's eggs, then 
% this cuckoo's egg is less likely to be discovered, thus the fitness should 
% be related to the difference in solutions.  Therefore, it is a good idea 
% to do a random walk in a biased way with some random step sizes.  
%% New solution by biased/selective random walks
stepsize=rand*(nest(randperm(n),:)-nest(randperm(n),:));
new_nest=nest+stepsize.*K;
for j=1:size(new_nest,1)
    s=new_nest(j,:);
  new_nest(j,:)=simplebounds(s,Lb,Ub);  
end

% Application of simple constraints
function s=simplebounds(s,Lb,Ub)
  % Apply the lower bound
  ns_tmp=s;
  I=ns_tmp<Lb;
  ns_tmp(I)=Lb(I);
  
  % Apply the upper bounds 
  J=ns_tmp>Ub;
  ns_tmp(J)=Ub(J);
  % Update this new move 
  s=ns_tmp;

%%  A d-dimensional objective function
function totalPenalty=evalModel(CtrlParams)
global f w_e w_n bw floorcoeff Kp_v Ki_v Kp_i Ki_i Kr_i1 Kr_i2 Kr_i3
autoSim = true;
CtrlParams = u;
modelCfg

