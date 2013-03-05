% This file is for adding directories to the Matlab search path.
% calls to addpath here will place directories to the top of the 
% Matlab search path.
%
% Example:
%   
%     addpath( 'add/your/path/')

addpath ~/sand/matlab/

% This adds <Presto_root>/src/matlab to the matlab search path: 
PRESTO_ROOT=[fileparts(mfilename('fullpath')), filesep];
presto_relpathlist={'', 'src/matlab'};
for k=1:numel(presto_relpathlist)
    tmp=[PRESTO_ROOT,presto_relpathlist{k}];
    fprintf('adding path %s\n',tmp);
    addpath(tmp);
end
