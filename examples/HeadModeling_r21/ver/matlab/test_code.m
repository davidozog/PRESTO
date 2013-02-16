% Test code, after making changes to the code 


clear *

hmenv = getenv('HEAD_MODELING_HOME');
verPath = fullfile(hmenv, 'ver/solution/', '');
testPath = fullfile(hmenv, 'test/solution/', '');

%Forward testing
SPH_HM_ADICUDAISO_100                = 0; %
SPH_HM_ADIOMPISO_100                 = 0;
SPH_HM_VAICUDAISO_100                = 0;
SPH_HM_VAIOMPISO_100                 = 0;
SPH_HM_VAICUDAANI_100                = 0;
SPH_HM_VAIOMPANI_100                 = 0; %

SPH_HM_ADICUDAISO_EIT_200            = 0;
SPH_HM_ADIOMPISO_EIT_200             = 0;
SPH_HM_VAICUDAISO_EIT_200            = 0;
SPH_HM_VAIOMPISO_EIT_200             = 0;
SPH_HM_VAICUDAANI_200                = 0;
SPH_HM_VAIOMPANI_200                 = 0;

PL_BK_ADICUDAISO_1MM                 = 0; %
PL_BK_ADIOMPISO_1MM                  = 0;
PL_BK_VAICUDAISO_1MM                 = 0; %
PL_BK_VAIOMPISO_1MM                  = 0;

% same as above just using 6 dipoles and 10 sensors
CH_BK_ADICUDAISO_LFMTRIPREC_6D_1MM         = 0;
CH_BK_ADICUDAISO_LFMTRIPFOR_6D_1MM         = 0;
CH_BK_VAICUDAISO_LFMTRIPFOR_6D_1MM         = 0;
CH_BK_VAICUDAISO_LFMTRIPREC_6D_1MM         = 0;
CH_BK_ADICUDAISO_LFMTRIPREC_6D_ENDIANN_1MM = 0;

%Conductivity testing
CH_BK_ADICUDAISO_CONDSIM_2MM         = 1;
CH_BK_ADICUDAISO_CONDSA_2MM          = 1;
CH_BK_VAICUDAISO_CONDSA_2MM          = 1;
CH_BK_VAICUDAISO_CONDSIM_2MM         = 1;
CH_BK_VAICUDAANI_CONDSA_2MM          = 1;
CH_BK_VAICUDAANI_CONDSIM_2MM         = 1;

pau = 0;
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
if SPH_HM_ADICUDAISO_100
	%figure
	fprintf('SPH_HM_ADICUDAISO_100 ... ');
	try
		tadic = load ([testPath, 'sph_hm_adicudaiso_100_output/', ...
			'sph_hm_adicudaiso_100_sns.txt']);
		vadic = load ([verPath,  'sph_hm_adicudaiso_100_output/', ...
			'sph_hm_adicudaiso_100_sns.txt']);
		plot(vadic(:, 1), vadic(:, 2), 'or', tadic(:, 1), tadic(:, 2), '*b');
		title('SPH-HM-ADICUDAISO-100');
		
		fprintf('done\n');

	catch err
		fprintf('error \n');
		
	end
	pause
end

if SPH_HM_ADIOMPISO_100
	%figure
	fprintf('SPH_HM_ADIOMPISO_100 ... ');
	try
	tadio = load ([testPath, 'sph_hm_adiompiso_100_output/', ...
		'sph_hm_adiompiso_100_sns.txt']);
	vadio = load ([verPath, 'sph_hm_adiompiso_100_output/', ...
		'sph_hm_adiompiso_100_sns.txt']);
	plot(vadio(:, 1), vadio(:, 2), 'or', tadio(:, 1), tadio(:, 2), '*b');
	title('SPH-HM-ADIOMPISO-100');
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
end

if SPH_HM_VAICUDAISO_100
	%figure
	fprintf('SPH_HM_ADICUDAISO_100 ... ');
	try
	tvaic = load ([testPath, 'sph_hm_vaicudaiso_100_output/', ...
		'sph_hm_vaicudaiso_100_sns.txt']);
	vvaic = load ([verPath,  'sph_hm_vaicudaiso_100_output/', ...
		'sph_hm_vaicudaiso_100_sns.txt']);
	plot(vvaic(:, 1), vvaic(:, 2), 'or', tvaic(:, 1), tvaic(:, 2), '*b');
	title('SPH-HM-ADICUDAISO-100')
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
end

if SPH_HM_VAIOMPISO_100
	%figure
	fprintf('SPH_HM_VAIOMPISO_100 ... ');
	try
	tvaio = load ([testPath, 'sph_hm_vaiompiso_100_output/', ...
		'sph_hm_vaiompiso_100_sns.txt']);
	vvaio = load ([verPath, 'sph_hm_vaiompiso_100_output/', ...
		'sph_hm_vaiompiso_100_sns.txt']);
	plot(vvaio(:, 1), vvaio(:, 2), 'or', tvaio(:, 1), tvaio(:, 2), '*b');
	title('SPH-HM-VAIOMPISO-100');
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
end

if SPH_HM_VAICUDAANI_100
	fprintf('SPH_HM_VAICUDAANI_100 ... ');
	try
	tvac = load ([testPath, 'sph_hm_vaicudaani_100_output/', ...
		'sph_hm_vaicudaani_100_sns.txt']);
	vvac = load ([verPath, 'sph_hm_vaicudaani_100_output/', ...
		'sph_hm_vaicudaani_100_sns.txt']);
	plot(vvac(:, 1), vvac(:, 2), 'or', tvac(:, 1), tvac(:, 2), '*b');
	title('SPH-HM-VAICUDAANI-100');
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause

end

if SPH_HM_VAIOMPANI_100
	fprintf('SPH_HM_VAIOMPANI_100 ... ');
	try
	tvao = load ([testPath, 'sph_hm_vaiompani_100_output/', ...
		'sph_hm_vaiompani_100_sns.txt']);
	vvao = load ([verPath,  'sph_hm_vaiompani_100_output/', ...
		'sph_hm_vaiompani_100_sns.txt']);
	plot(vvao(:, 1), vvao(:, 2), 'or', tvao(:, 1), tvao(:, 2), '*b');
	title('SPH-HM-VAIOMPANI-100');
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
end

if SPH_HM_ADICUDAISO_EIT_200 
	%figure
	fprintf('SPH_HM_ADICUDAISO_EIT_200 ... ');
	
	try
	tadc = load ([testPath, 'sph_hm_adicudaiso_eit_200_output/', ...
		'sph_hm_adicudaiso_eit_200_sns.txt']);
	tadc(tadc(:, 1) == 61, 2) = 0;
	tadc(tadc(:, 1) == 67, 2) = 0;
	
	vadc = load ([verPath, 'sph_hm_adicudaiso_eit_200_output/', ...
		'sph_hm_adicudaiso_eit_200_sns.txt']);
	vadc(vadc(:, 1) == 61, 2) = 0;
	vadc(vadc(:, 1) == 67, 2) = 0;
	
	plot(vadc(:, 1), vadc(:, 2), 'or',  tadc(:, 1), tadc(:, 2), '*b');
	title('SPH-HM-ADICUDAISO-EIT-200');
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end

if SPH_HM_ADIOMPISO_EIT_200	
	%figure
	fprintf('SPH_HM_ADIOMPISO_EIT_200 ... ');
	try
	tado = load ([testPath, 'sph_hm_adiompiso_eit_200_output/', ...
		'sph_hm_adiompiso_eit_200_sns.txt']);
	tado(tado(:, 1) == 61, 2) = 0;
	tado(tado(:, 1) == 67, 2) = 0;

	vado = load ([verPath, 'sph_hm_adiompiso_eit_200_output/', ...
		'sph_hm_adiompiso_eit_200_sns.txt']);
	vado(vado(:, 1) == 61, 2) = 0;
	vado(vado(:, 1) == 67, 2) = 0;

	plot(vado(:, 1), vado(:, 2), 'or',  tado(:, 1), tado(:, 2), '*b');
	title('SPH-HM-ADIOMPISO-EIT-200');
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause;
	
	
end
	
if SPH_HM_VAICUDAISO_EIT_200
	
	%figure
	fprintf('SPH_HM_VAICUDAISO_EIT_200 ... ');
	
	try
	tvac = load ([testPath, 'sph_hm_vaicudaiso_eit_200_output/', ...
		'sph_hm_vaicudaiso_eit_200_sns.txt']);
	tvac(tvac(:, 1) == 61, 2) = 0;
	tvac(tvac(:, 1) == 67, 2) = 0;

	vvac = load ([verPath, 'sph_hm_vaicudaiso_eit_200_output/', ...
		'sph_hm_vaicudaiso_eit_200_sns.txt']);
	vvac(vvac(:, 1) == 61, 2) = 0;
	vvac(vvac(:, 1) == 67, 2) = 0;

	plot(vvac(:, 1), vvac(:, 2), 'or',  tvac(:, 1), tvac(:, 2), '*b');
	title('SPH-HM-VAICUDAISO-EIT-200');
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end

if SPH_HM_VAIOMPISO_EIT_200
	
	%figure
	fprintf('SPH_HM_VAIOMPISO_EIT_200 ... ');
	try
	tvao = load ([testPath, 'sph_hm_vaiompiso_eit_200_output/', ...
		'sph_hm_vaiompiso_eit_200_sns.txt']);
	tvao(tvao(:, 1) == 61, 2) = 0;
	tvao(tvao(:, 1) == 67, 2) = 0;

	vvao = load ([verPath, 'sph_hm_vaiompiso_eit_200_output/', ...
		'sph_hm_vaiompiso_eit_200_sns.txt']);
	vvao(tvao(:, 1) == 61, 2) = 0;
	vvao(tvao(:, 1) == 67, 2) = 0;

	plot(vvao(:, 1), vvao(:, 2), 'or',  tvao(:, 1), tvao(:, 2), '*b');
	title('SPH-HM-VAIOMPISO-EIT-200');
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end

if SPH_HM_VAICUDAANI_200
	fprintf('SPH_HM_VAICUDAANI_200 ... ')
	try
	tvac = load ([testPath, 'sph_hm_vaicudaani_200_output/', ...
		'sph_hm_vaicudaani_200_sns.txt']);
	vvac = load ([verPath, 'sph_hm_vaicudaani_200_output/', ...
		'sph_hm_vaicudaani_200_sns.txt']);
	plot(vvac(:, 1), vvac(:, 2), 'or', tvac(:, 1), tvac(:, 2), '*b');
	title('SPH-HM-VAICUDAANI-200')
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
end

if SPH_HM_VAIOMPANI_200
	fprintf('SPH_HM_VAIOMPANI_200 ... ');
	try
	tvao = load ([testPath, 'sph_hm_vaiompani_200_output/', ...
		'sph_hm_vaiompani_200_sns.txt']);
	vvao = load ([verPath, 'sph_hm_vaiompani_200_output/', ...
		'sph_hm_vaiompani_200_sns.txt']);
	plot(vvao(:, 1), vvao(:, 2), 'or', tvao(:, 1), tvao(:, 2), '*b');
	title('SPH-HM-VAIOMPANI-200');
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end

	pause
end

if PL_BK_ADICUDAISO_1MM
fprintf('PL_BK_ADICUDAISO_1MM ... ')
try
	tadic = load ( [testPath, 'pl_bk_adicudaiso_1mm_output/', ...
		'pl_bk_adicudaiso_1mm_sns.txt'] );
	tadic(tadic(:, 1) == 4,  2)  = 0;
	tadic(tadic(:, 1) == 12, 2)  = 0;

	vadic = load ( [verPath, 'pl_bk_adicudaiso_1mm_output/', ...
		'pl_bk_adicudaiso_1mm_sns.txt'] );
	vadic(vadic(:, 1) == 4,  2)  = 0;
	vadic(vadic(:, 1) == 12, 2)  = 0;
	plot(vadic(:, 1), vadic(:, 2), 'or', tadic(:, 1), tadic(:, 2), '*b');
	title('PL-BK-ADICUDAISO-1MM');
	
	fprintf('done\n');
	catch err
		fprintf('error \n');

	end

	pause

end

if PL_BK_ADIOMPISO_1MM
	fprintf('PL_BK_ADIOMPISO_1MM ... ')
	try
	tadio = load ( [testPath, 'pl_bk_adiompiso_1mm_output/', ...
		'pl_bk_adiompiso_1mm_sns.txt' ] );
	tadio(tadio(:, 1) == 4,  2)  = 0;
	tadio(tadio(:, 1) == 12, 2)  = 0;

	vadio = load ( [verPath, 'pl_bk_adiompiso_1mm_output/', ...
		'pl_bk_adiompiso_1mm_sns.txt' ] );
	vadio(vadio(:, 1) == 4,  2)  = 0;
	vadio(vadio(:, 1) == 12, 2)  = 0;
	plot(vadio(:, 1), vadio(:, 2), 'or', vadio(:, 1), vadio(:, 2),'*b');
	title('PL-BK-ADIOMPISO-1MM');
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
end

if PL_BK_VAICUDAISO_1MM
	fprintf('PL_BK_VAICUDAISO_1MM ... ')
	try
	tvaic = load ( [testPath, 'pl_bk_vaicudaiso_1mm_output/', ...
		'pl_bk_vaicudaiso_1mm_sns.txt'] );
	tvaic(tvaic(:, 1) == 4,  2)  = 0;
	tvaic(tvaic(:, 1) == 12, 2)  = 0;

	vvaic = load ( [verPath,  'pl_bk_vaicudaiso_1mm_output/', ...
		'pl_bk_vaicudaiso_1mm_sns.txt'] );
	vvaic(vvaic(:, 1) == 4,  2)  = 0;
	vvaic(vvaic(:, 1) == 12, 2)  = 0;
	plot( vvaic(:, 1), vvaic(:,2), 'or', tvaic(:,1), tvaic(:,2), '*b');
	title('PL-BK-VAICUDAISO-1MM ');
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
end

if PL_BK_VAIOMPISO_1MM
	fprintf('PL_BK_VAIOMPISO_1MM ... ')
	try
	tvaio = load ( [testPath, 'pl_bk_vaiompiso_1mm_output/', ...
		'pl_bk_vaiompiso_1mm_sns.txt' ] );
	tvaio(tvaio(:, 1) == 4,  2)  = 0;
	tvaio(tvaio(:, 1) == 12, 2)  = 0;

	vvaio = load ( [verPath, 'pl_bk_vaiompiso_1mm_output/', ...
		'pl_bk_vaiompiso_1mm_sns.txt' ] );
	vvaio(vvaio(:, 1) == 4,  2)  = 0;
	vvaio(vvaio(:, 1) == 12, 2)  = 0;
	plot( vvaio(:, 1), vvaio(:,2), 'or', tvaio(:,1), tvaio(:,2), '*b');
	title('PL-BK-VAIOMPISO-1MM');
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
end

%{
if CH_BK_VAICUDAISO_LFMFOR_1MM
	fprintf('CH_BK_VAICUDAISO_LFMFOR_1MM ... ')
	try
	tc = load([testPath, 'ch_bk_vaicudaiso_lfmfor_1mm_output/', ...
		'ch_bk_vaicudaiso_lfmfor_1mm_sns.txt']);
	vc = load([verPath, 'ch_bk_vaicudaiso_lfmfor_1mm_output/', ...
		'ch_bk_vaicudaiso_lfmfor_1mm_sns.txt']);
	plot(vc(:, 1), vc(:, 2), 'or', tc(:, 1), tc(:, 2), '*b');
	fprintf('done\n');
		catch err
		fprintf('error \n');
	end

	pause

end

if CH_BK_VAIOMPISO_LFMFOR_1MM
	fprintf('CH_BK_VAIOMPISO_LFMFOR_1MM ... ')
try
	to = load([testPath, 'ch_bk_vaiompiso_lfmfor_1mm_output/', ...
		'ch_bk_vaiompiso_lfmfor_1mm_sns.txt']);
	vo = load([verPath, 'ch_bk_vaiompiso_lfmfor_1mm_output/', ...
		'ch_bk_vaiompiso_lfmfor_1mm_sns.txt']);
	plot(to(:, 1), to(:, 2), 'or', vo(:, 1), vo(:, 2), '*b');
	fprintf('done\n');
	catch err
		fprintf('error \n');
end
	pause

end
%}



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% all verification data using 3 process on Frankenstien 
%{
if CH_BK_ADICUDAISO_LFMTRIPREC_1MM
	fprintf('CH_BK_ADICUDAISO_LFMTRIPREC_1MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_adicudaiso_lfmtriprec_1mm_output/', ...
		'ch_bk_adicudaiso_lfmtriprec_1mm_trip.gs'], 'r');
	tg = fread(fid, 'double', 'l');
	tg = reshape(tg, 60, 41)';
	fclose(fid);

	fid = fopen([verPath,  'ch_bk_adicudaiso_lfmtriprec_1mm_output/', ...
		'ch_bk_adicudaiso_lfmtriprec_1mm_trip.gs'], 'r');
	vg = fread(fid, 'double', 'l');
	vg = reshape(vg, 60, 41)';
	fclose(fid);
	
	npoints = 4;
	dips = randi(size(tg, 2), npoints); 
	
	for i=1:npoints
		plot(1:size(vg, 1), vg(:, dips(i)), 'or', 1:size(tg, 1), ... 
			tg(:, dips(i)), '*b');
		
		tit = sprintf('dipole %d', dips(i));
		title(tit);
		legend('Test.Adi.Rec', 'Ver.Adi.Rec')
		pause(.5)
	end
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end

if CH_BK_ADICUDAISO_LFMTRIPFOR_1MM
	fprintf('CH_BK_ADICUDAISO_LFMTRIPFOR_1MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_adicudaiso_lfmtripfor_1mm_output/', ...
		'ch_bk_adicudaiso_lfmtripfor_1mm_trip.gs'], 'r');
	tg = fread(fid, 'double', 'l');
	tg = reshape(tg, 60, 41)';
	fclose(fid);

	fid = fopen([verPath, 'ch_bk_adicudaiso_lfmtripfor_1mm_output/', ...
		'ch_bk_adicudaiso_lfmtripfor_1mm_trip.gs'], 'r');
	vg = fread(fid, 'double', 'l');
	vg = reshape(vg, 60, 41)';
	fclose(fid);
	
	npoints = 4;
	dips = randi(size(tg, 2), npoints); 
	
	for i=1:npoints
		plot(1:size(vg, 1), vg(:, dips(i)), 'or', 1:size(tg, 1), tg(:, dips(i)), '*b');
		tit = sprintf('dipole %d', dips(i));
		title(tit);
		legend('Test.Adi.For', 'Ver.Adi.For')
		pause(.5)
	end
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end

if CH_BK_VAICUDAISO_LFMTRIPREC_1MM
	fprintf('CH_BK_VAICUDAISO_LFMTRIPREC_1MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_vaicudaiso_lfmtriprec_1mm_output/', ...
		'ch_bk_vaicudaiso_lfmtriprec_1mm_trip.gs'],'r');
	tg = fread(fid, 'double', 'l');
	tg = reshape(tg, 60, 41)';
	fclose(fid);

	fid = fopen([verPath, 'ch_bk_vaicudaiso_lfmtriprec_1mm_output/', ...
		'ch_bk_vaicudaiso_lfmtriprec_1mm_trip.gs'], 'r');
	vg = fread(fid, 'double', 'l');
	vg = reshape(vg, 60, 41)';
	fclose(fid);

	npoints = 4;
	dips = randi(size(tg, 2), npoints); 
	
	for i=1:npoints
		plot(1:size(tg, 1), tg(:, dips(i)), 'or', 1:size(vg, 1), vg(:, dips(i)), '*b');
		tit = sprintf('dipole %d', dips(i));
		title(tit);
		legend('Test.Vai.Rec', 'Ver.Vai.Rec')
		pause(.5)
	end
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause

end

if CH_BK_VAICUDAISO_LFMTRIPFOR_1MM
	fprintf('CH_BK_VAICUDAISO_LFMTRIPFOR_1MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_vaicudaiso_lfmtripfor_1mm_output/', ...
		'ch_bk_vaicudaiso_lfmtripfor_1mm_trip.gs'], 'r');
	tg = fread(fid, 'double', 'l');
	tg = reshape(tg, 60, 41)';

	fid = fopen([verPath,'ch_bk_vaicudaiso_lfmtripfor_1mm_output/', ...
		'ch_bk_vaicudaiso_lfmtripfor_1mm_trip.gs'], 'r');
	vg  = fread(fid, 'double', 'l');
	vg  = reshape(vg, 60, 41)';
	
	npoints = 4;
	dips = randi(size(tg, 2), npoints); 
	
	for i=1:npoints
		plot(1:size(vg, 1), vg(:, i), 'or', 1:size(tg, 1), tg(:, i), '*b');
		tit = sprintf('dipole %d', dips(i));
		title(tit);
		legend('Test.Vai.For', 'Ver.Vai.For')
		pause(.5)
	end
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end
%}

%%%%%%%%%%%%% smaller lfm 6dipoles and 10 sensors %%%%%%%%%%%%
if CH_BK_ADICUDAISO_LFMTRIPREC_6D_1MM
	fprintf('CH_BK_ADICUDAISO_LFMTRIPREC_6D_1MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_adicudaiso_lfmtriprec_6d_1mm_output/', ...
		'ch_bk_adicudaiso_lfmtriprec_6d_1mm_trip.gs'], 'r');
	tg = fread(fid, 'double', 'l');
	fclose(fid);

	fid = fopen([verPath, 'ch_bk_adicudaiso_lfmtriprec_6d_1mm_output/', ...
		'ch_bk_adicudaiso_lfmtriprec_6d_1mm_trip.gs'], 'r');
	vg = fread(fid, 'double', 'l');
	fclose(fid);

	plot(1:size(vg, 1), vg, 'or', 1:size(tg, 1), tg, '*b');
  hold on
  plot(1:size(vg, 1), tg - vg, 'g')
  hold off
	title('');
		
	title('CH-BK-ADICUDAISO-LFMTRIPREC_6D-1MM');
	legend('Test.Adi.Rec', 'Ver.Adi.Rec')

	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end

if CH_BK_ADICUDAISO_LFMTRIPFOR_6D_1MM
	fprintf('CH_BK_ADICUDAISO_LFMTRIPFOR_6D_1MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_adicudaiso_lfmtripfor_6d_1mm_output/', ...
		'ch_bk_adicudaiso_lfmtripfor_6d_1mm_trip.gs'], 'r');
	tg = fread(fid, 'double', 'l');
	fclose(fid);

	fid = fopen([verPath, 'ch_bk_adicudaiso_lfmtripfor_6d_1mm_output/', ...
		'ch_bk_adicudaiso_lfmtripfor_6d_1mm_trip.gs'], 'r');
	vg = fread(fid, 'double', 'l');
	fclose(fid);
	

	plot(1:size(vg, 1), vg, 'or', 1:size(tg, 1), tg, '*b');
  hold on
	plot(1:size(vg, 1), tg-vg, 'g');
	title('CH-BK-ADICUDAISO-LFMTRIPFOR-6D-1MM');

	legend('Test.Adi.For', 'Ver.Adi.For')
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end

if CH_BK_VAICUDAISO_LFMTRIPREC_6D_1MM
	fprintf('CH_BK_VAICUDAISO_LFMTRIPREC_6D_1MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_vaicudaiso_lfmtriprec_6d_1mm_output/', ...
		'ch_bk_vaicudaiso_lfmtriprec_6d_1mm_trip.gs'],'r');
	tg = fread(fid, 'double', 'l');
	fclose(fid);

	fid = fopen([verPath, 'ch_bk_vaicudaiso_lfmtriprec_6d_1mm_output/', ...
		'ch_bk_vaicudaiso_lfmtriprec_6d_1mm_trip.gs'], 'r');
	vg = fread(fid, 'double', 'l');
	fclose(fid);

	plot(1:size(tg, 1), tg, 'or', 1:size(vg, 1), vg, '*b');
	title('CH-BK-VAICUDAISO-LFMTRIPREC_6D-1MM');
	legend('Test.Vai.Rec', 'Ver.Vai.Rec')
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause

end

if CH_BK_VAICUDAISO_LFMTRIPFOR_6D_1MM
	fprintf('CH_BK_VAICUDAISO_LFMTRIPFOR_6D_1MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_vaicudaiso_lfmtripfor_6d_1mm_output/', ...
		'ch_bk_vaicudaiso_lfmtripfor_6d_1mm_trip.gs'], 'r');
	tg = fread(fid, 'double', 'l');
	fclose(fid);

	fid = fopen([verPath,'ch_bk_vaicudaiso_lfmtripfor_6d_1mm_output/', ...
		'ch_bk_vaicudaiso_lfmtripfor_6d_1mm_trip.gs'], 'r');
	vg  = fread(fid, 'double', 'l');
	fclose(fid);
	
	plot(1:size(vg, 1), vg, 'or', 1:size(tg, 1), tg, '*b');
	
	title('CH-BK-VAICUDAISO-LFMTRIPFOR-6D-1MM ');
	legend('Test.Vai.For', 'Ver.Vai.For')
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end

if CH_BK_ADICUDAISO_LFMTRIPREC_6D_ENDIANN_1MM
	fprintf('CH_BK_ADICUDAISO_LFMTRIPREC_6D_ENDIANN_1MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_adicudaiso_lfmtriprec_6d_endiann_1mm_output/', ...
		'ch_bk_adicudaiso_lfmtriprec_6d_endiann_1mm_trip.gs'], 'r');
	tg = fread(fid, 'double', 'b');
	fclose(fid);

	fid = fopen([verPath, 'ch_bk_adicudaiso_lfmtriprec_6d_1mm_output/', ...
		'ch_bk_adicudaiso_lfmtriprec_6d_1mm_trip.gs'], 'r');
	vg = fread(fid, 'double', 'l');
	fclose(fid);

	plot(1:size(vg, 1), vg, 'or', 1:size(tg, 1), 1000*tg, '*b');
		
	title('CH-BK-ADICUDAISO-LFMTRIPREC-6D-ENDIANN-1MM');
	legend('Test.Adi.Rec', 'Ver.Adi.Rec')

	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end



%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if CH_BK_ADICUDAISO_CONDSIM_2MM
	
	fprintf('CH_BK_ADICUDAISO_CONDSIM_2MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_adicudaiso_condsim_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58.cio_0'], 'r');
	tdat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
	fclose(fid);

	fid = fopen([verPath, 'ch_bk_adicudaiso_condsim_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58.cio_0'], 'r');
	
	vdat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
	fclose(fid);

	ii = [4, 5, 6, 7, 8];
	
	npoints = size(tdat{1}(:), 1);
	if npoints > 100
		npoints = 100;
	end
	
	for i=1:size(ii, 2)
		j = ii(i);	
		plot(1:npoints, vdat{j}(1:npoints), 'or', ...
			1:npoints, tdat{j}(1:npoints), '*b')
		pause;
	end
	title('CH-BK-ADICUDAISO-CONDSIM-2MM');
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
    %[p, n, r, objt, objo, tc1, tc2, tc3, s, oc1, oc2, oc3]
    pause

end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%% conductivity test sa

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if CH_BK_ADICUDAISO_CONDSA_2MM
	% verification data generated on Frankenstien using 2 procss
	fprintf('CH_BK_ADICUDAISO_CONDSA_2MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_adicudaiso_condsa_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58.cio_0'], 'r');
	tdat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
	fclose(fid);
	
	fid = fopen([verPath, 'ch_bk_adicudaiso_condsa_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58.cio_0'], 'r');
	vdat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
	fclose(fid);
	
	ii = [4, 5, 6, 7, 8];
	npoints = size(tdat{1}(:), 1);
	if npoints > 100
		npoints = 100;
	end
	
	for i=1:size(ii, 2)
		j = ii(i);
		plot(1:npoints, vdat{j}(1:npoints), 'or', ...
			1:npoints, tdat{j}(1:npoints), '*b')
		title('CH-BK-ADICUDAISO-CONDSA-2MM ');
		pause;
	end
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause

end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if CH_BK_VAICUDAISO_CONDSA_2MM
	% verification data generated on Frankenstien using 2 procss
	fprintf('CH_BK_VAICUDAISO_CONDSA_2MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_vaicudaiso_condsa_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58.cio_0'], 'r');
	tdat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
	fclose(fid);
	
	fid = fopen([verPath, 'ch_bk_vaicudaiso_condsa_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58.cio_0'], 'r');
	vdat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
	fclose(fid);
	
	ii = [4, 5, 6, 7, 8];
	
	npoints = size(tdat{1}(:), 1);
	if npoints > 100
		npoints = 100;
	end
	
	for i=1:size(ii, 2)
		j = ii(i);
		plot(1:npoints, vdat{j}(1:npoints), 'or', ...
			1:npoints, tdat{j}(1:npoints), '*b')
		title('CH-BK-VAICUDAISO-CONDSA-2MM');
		pause;
	end
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
	
end

if CH_BK_VAICUDAISO_CONDSIM_2MM
	% verification data generated on Frankenstien using 2 procss
	
	fprintf('CH_BK_VAICUDAISO_CONDSIM_2MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_vaicudaiso_condsim_simulated_2mm_output', ... 
		'/ch_simmeas_i117_s58.cio_0'], 'r');
	tdat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
	fclose(fid);
	
	fid = fopen([verPath, 'ch_bk_vaicudaiso_condsim_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58.cio_0'], 'r');
	vdat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
	fclose(fid);
	
	ii = [4, 5, 6, 7, 8];
	
	npoints = size(tdat{1}(:), 1);
	if npoints > 100
		npoints = 100;
	end
	
	for i=1:size(ii, 2)
		j = ii(i);
		plot(1:npoints, vdat{j}(1:npoints), 'or', ...
			1:npoints, tdat{j}(1:npoints), '*b')
		title('CH-BK-VAICUDAISO-CONDSIM-2MM');
		pause;
	end
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
		
end

if CH_BK_VAICUDAANI_CONDSIM_2MM
	% verification data generated on Frankenstien using 2 procss
	fprintf('CH_BK_VAICUDAANI_CONDSIM_2MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_vaicudaani_condsim_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58_ani.cio_0'], 'r');
	tdat = textscan(fid, '%d %d %d %f %f %f %f %f %f %s %f %f %f %f');
	fclose(fid);
	
	fid = fopen([verPath, 'ch_bk_vaicudaani_condsim_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58_ani.cio_0'], 'r');
	vdat = textscan(fid, '%d %d %d %f %f %f %f %f %f %s %f %f %f %f');
	fclose(fid);
	
	ii = [4, 5, 6, 7, 8];
	
	npoints = size(tdat{1}(:), 1);
	if npoints > 100
		npoints = 100;
	end
	
	for i=1:size(ii, 2)
		j = ii(i);
		plot(1:npoints, vdat{j}(1:npoints), 'or', ...
			1:npoints, tdat{j}(1:npoints), '*b')
		title('CH-BK-VAICUDAANI-CONDSIM-2MM');
		pause;
	end
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
end

if CH_BK_VAICUDAANI_CONDSA_2MM
	% verification data generated on Frankenstien using 2 procss
	
	fprintf('CH-BK-VAICUDAANI-CONDSA-2MM ... ')
	try
	fid = fopen([testPath, 'ch_bk_vaicudaani_condsa_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58_ani.cio_0'], 'r');
	                        
	tdat = textscan(fid, '%d %d %d %f %f %f %f %f %f %s %f %f %f %f');
	fclose(fid);
	
	fid = fopen([verPath, 'ch_bk_vaicudaani_condsa_simulated_2mm_output', ...
		'/ch_simmeas_i117_s58_ani.cio_0'], 'r');
	                       
	vdat = textscan(fid, '%d %d %d %f %f %f %f %f %f %s %f %f %f %f');
	fclose(fid);
	
	ii = [4, 5, 6, 7, 8];
	
	npoints = size(tdat{1}(:), 1);
	if npoints > 100
		npoints = 100;
	end
	
	for i=1:size(ii, 2)
		j = ii(i);
		plot(1:npoints, vdat{j}(1:npoints), 'or', ...
			1:npoints, tdat{j}(1:npoints), '*b')
		title('CH-BK-VAICUDAANI-CONDSA-2MM');
		pause;
	end
	
	fprintf('done\n');
	catch err
		fprintf('error \n');
	end
	pause
end
