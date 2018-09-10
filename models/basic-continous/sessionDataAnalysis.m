sessionSaveFile = 'sessionData/Rev12_runSummary.mat';
if exist('rev12_sessionData','var') ~= 1
    load(sessionSaveFile);    
end

dataSize = length(rev12_sessionData);
minScore = inf;
minCfg=[];
minCount = 0;
for row = 1:dataSize
   thisRowScore = rev12_sessionData{row,2};
   thisScore = thisRowScore(1);
   if thisScore < minScore
        minCount=minCount +1;
       minScore = thisScore;
       minCfg = {row rev12_sessionData{row,1} thisRowScore};
    
       %minScore
        disp(['Encounter' 'Sim Seq']);
        disp([minCount minCfg{1}])
        disp('Cfg');
        disp(minCfg{2})
        disp('Results');
        disp(minCfg{3})
       
   end
end
