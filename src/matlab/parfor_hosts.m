% Getting the machine host name

[~,hostname] = system('hostname');

% If the loop iterations are the same as the size of matlabpool, the
% command is run once per worker.

parfor ix = 1:matlabpool('size')
    [~,hostnameID{ix}] = system('hostname');
end

% Can then do host/machine specific commands
hostnames = unique(hostnameID);
checkhost = hostnames(1);

parfor ix = 1:matlabpool('size')
    [~,myhost] = system('hostname');
    if strcmp(myhost,checkhost)
       display('On Machine 1')
    else
        display('NOT on Machine 1')
    end
end
