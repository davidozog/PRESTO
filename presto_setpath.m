% fileparts(mfilename('fullpath')) returns directory containing this .m file
PRESTO_ROOT=[fileparts(mfilename('fullpath')), filesep];
presto_relpathlist={'', 'src/matlab'};
for k=1:numel(presto_relpathlist)
    tmp=[PRESTO_ROOT,presto_relpathlist{k}];
    fprintf('adding path %s\n',tmp);
    addpath(tmp);
end
