function [] = launch_workers(func_call, split_struct, shared_struct)

%     lparen = strfind(func_call, '(');
%     method = strtrim(func_call(1:lparen-1));
%     remain = func_call(lparen+1:end);
%     static_args = {}; i = 1;
%     while (remain)
%         [token, remain] = strtok(remain, ',');
%         token = strtrim(token);
%         if token(end) ~= ')'
%             static_args = [static_args token];
%         else 
%             static_args = [static_args token(1:end-1)];
%             remain = [];
%         end
%     end

    method = func_call;
    
    save('split_data.mat', 'split_struct')
    save('shared_data.mat', 'shared_struct')
    
    [status, result] = system(horzcat('matlabyrinth ', method, ' ', ...
                                 'split_data.mat ', 'shared_data.mat'));

result
