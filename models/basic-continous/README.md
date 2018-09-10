# Basic Continuous-Time Model Check Point
Date: 2018 March 1
---
## Interactive Simulation
First, run `Rev12_DCtoAC_Tuning` from console to open the Simulink model

Run `modelCfg.m` for a single run, which loads the necessary parameters into the base workspace.
This will open up bode plots which shows the parameters given. To change the config, set the 
`CtrlParams` vector before running the script. 

For adjusting `CtrlParams`, refer to the different sections of the script.

The results of the simulation is calculated as a penalty saved within the `sessionData/` folder 
for analysis.

## Random Walk Search
Run `param_search.m` to initiate a random walk search using the CS algorithm. The search space is
set by the upper and lower bounds (`Ub`, `Lb`) in the file. 

After each iteration of the random walk (involving `n=25` simulations), the search state is saved 
in `cs_resume.mat`. If this file is found, the search from resume from this state. IF this is not 
desired (e.g. if `Lb`,`Ub` or `n` have changed), remove this `cs_resume.mat` before start running.

## Hinf
An example of the Hinf shaping freq response has been included for further fine-tuning. 
These tends to be of higher order than the random walk optimsation results.

# TODO


## Model Efficiency
- Concurrency in model
- Fast Restart
- reduce consecutive zero crossing

## Model Feature
- Model Startup
- Score step response shape
- State space model of switching circuit