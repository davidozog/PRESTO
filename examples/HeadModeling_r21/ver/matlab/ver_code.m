% Cross verification of the several forward solver 
% Verification of the simulated annealing and simplex search
% Verification of LFM generation using reciprocity and forward calculation
% All data are generated using the input data files in 
% $HEADMODELING_HOME/ver/input

clear *
hmenv = getenv('HEAD_MODELING_HOME');
dataPath = fullfile(hmenv, 'ver/solution/', '');
%dataPath = '/home/users/adnan/LocalProjects/HeadModeling/vai_trunk/ver/solution/';

SPH_ISO_200                      = 1;
SPH_ISO_100                      = 1;
SPH_ANI_100                      = 1;
SPH_ANI_200                      = 1;
PL_ADIVAI_1MM                    = 1;
CH_LFM_TRIP_1MM                  = 1;
CH_COND_ADI_SIM_2MM              = 1; %%
CH_COND_ADI_SA_2MM               = 1; %%
CH_COND_VAIISO_SIM_2MM           = 1;
CH_COND_VAIISO_SA_2MM            = 1; %%
CH_COND_VAIANI_SA_2MM            = 1;
CH_COND_VAIANI_SIM_2MM           = 1;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Use 100 voxel spherical data with demonck sphere
if SPH_ISO_100
	fprintf('SPH_ISO_100 ... ');
	figure(1)
	clf;
	try
		adic = load ([dataPath, 'sph_hm_adicudaiso_100_output/', ...
			'sph_hm_adicudaiso_100_sns.txt']);
	
		vaic = load ([dataPath, 'sph_hm_vaicudaiso_100_output/', ...
			'sph_hm_vaicudaiso_100_sns.txt']);

		adio = load ([dataPath, 'sph_hm_adiompiso_100_output/', ...
			'sph_hm_adiompiso_100_sns.txt']);
	
		vaio = load ([dataPath, 'sph_hm_vaiompiso_100_output/', ...
			'sph_hm_vaiompiso_100_sns.txt']);

		figure(1)
		plot(adio(:, 1), adio(:, 2), 'or');hold on
		plot(vaio(:, 1), vaio(:, 2), '*b');
		plot(adic(:, 1), adic(:, 2), 'og', 'markersize', 8);
		plot(vaic(:, 1), vaic(:, 2), 'sb', 'markersize', 12);
	
		legend('ADI-omp', 'VAI-omp', 'ADI-cuda', 'VAI-cuda');
		title('SPH_ISO_100');
		fprintf('done\n');
		
	catch err
		fprintf('error \n');
	end
	
	hold off
	pause
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% use spherical 200 voxel spherical data
if SPH_ISO_200
	fprintf('SPH_ISO_200 ... ');
	figure(1); clf;
	try
	
		sp  = load ([dataPath, 'sph_isosphiso_eit_200.txt']);
		adc = load ([dataPath, 'sph_hm_adicudaiso_eit_200_output/', ...
			'sph_hm_adicudaiso_eit_200_sns.txt']);
		ado = load ([dataPath, 'sph_hm_adiompiso_eit_200_output/', ...
			'sph_hm_adiompiso_eit_200_sns.txt']);
		vac = load ([dataPath, 'sph_hm_vaicudaiso_eit_200_output/', ...
			'sph_hm_vaicudaiso_eit_200_sns.txt']);
		vao = load ([dataPath, 'sph_hm_vaiompiso_eit_200_output/', ...
			'sph_hm_vaiompiso_eit_200_sns.txt']);

		sp(sp(:, 1) == 61, 2)  = 0;
		sp(sp(:, 1) == 67, 2) = 0;

		adc(sp(:, 1) == 61, 2) = 0;
		adc(sp(:, 1) == 67, 2) = 0;

		ado(sp(:, 1) == 61, 2) = 0;
		ado(sp(:, 1) == 67, 2) = 0;

		vac(sp(:, 1) == 61, 2) = 0;
		vac(sp(:, 1) == 67, 2) = 0;

		vao(sp(:, 1) == 61, 2) = 0;
		vao(sp(:, 1) == 67, 2) = 0;

		figure(1)
		plot(sp(:, 1), sp(:, 2), 'or'); hold on;
		plot(adc(:, 1), adc(:, 2), '*b');
		plot(ado(:, 1), ado(:, 2), 'sg', 'markersize', 8);
		legend('Sphere-200', 'ADI-cuda', 'ADI-omp');
		title('ADI-SPH-ISO-200');
		hold off
	
		pause
		figure(1)
		plot(sp(:, 1), sp(:, 2), 'or'); hold on;
		plot(vac(:, 1), vac(:, 2), '*b');
		plot(vao(:, 1), vao(:, 2), 'sg', 'markersize', 8);
		legend('Sphere-200', 'VAI-cuda', 'VAI-omp');
		title('VAI-SPH-ISO-200');
		fprintf('done\n');
	
	catch err
		fprintf('error \n');
	end
	
	hold off
	pause

end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% VAI anisotropic setting with 100 voxles 
if SPH_ANI_100
	figure(1); clf;
	fprintf('SPH_ANI_100 ... ');
	try
		sp  = load ([dataPath, 'sph_anisphani_100.txt']);
		vac = load ([dataPath, 'sph_hm_vaicudaani_100_output/', ...
			'sph_hm_vaicudaani_100_sns.txt']);
		vao = load ([dataPath, 'sph_hm_vaiompani_100_output/',  ...
			'sph_hm_vaiompani_100_sns.txt']);

		figure(1)
		plot(sp(:,1), sp(:, 2), 'or'); hold on
		plot(vao(:, 1), vao(:, 2), '*b');
		plot(vac(:, 1), vac(:, 2), 'sg', 'markersize', 8);
		legend('DeMonck', 'VAI-omp', 'VAI-cuda');
		title('SPH-ANI-100');
		fprintf('done\n');
	catch err
		fprintf('error \n');
	end	
	hold off
	pause
end
	
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% VAI anisotropic setting with 200 voxles **NOT done yet
if SPH_ANI_200
	fprintf('SPH_ANI_200 ... ');
	figure(1); clf;
	try
		sp  = load ([dataPath, 'sph_anisphani_200.txt']);
		vac = load ([dataPath, 'sph_hm_vaicudaani_200_output/', ...
			'sph_hm_vaicudaani_200_sns.txt']);
		vao = load ([dataPath, 'sph_hm_vaiompani_200_output/', ...
			'sph_hm_vaiompani_200_sns.txt']);

		figure(1)
		plot(sp(:,1), sp(:, 2), 'or'); hold on
		plot(vao(:, 1), vao(:, 2), '*b');
		plot(vac(:, 1), vac(:, 2), 'sg', 'markersize', 8);
		legend('DeMonck-200', 'VAI-omp-200', 'VAI-cuda-200');
		title('SPH-ANI-200');
		fprintf('done\n');
	catch err
		fprintf('error\n');
	end
	hold off
	pause
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% ADI pl 1mm data set
if PL_ADIVAI_1MM
	fprintf('PL_ADIVAI_1MM ... ');
	figure(1); clf;
	try
		adic = load ([dataPath, 'pl_bk_adicudaiso_1mm_output/', ...
			'pl_bk_adicudaiso_1mm_sns.txt']);
		adio = load ([dataPath, 'pl_bk_adiompiso_1mm_output/',  ...
			'pl_bk_adiompiso_1mm_sns.txt']);
		vaic = load ([dataPath, 'pl_bk_vaicudaiso_1mm_output/', ...
			'pl_bk_vaicudaiso_1mm_sns.txt']);
		vaio = load ([dataPath, 'pl_bk_vaiompiso_1mm_output/',  ...
			'pl_bk_vaiompiso_1mm_sns.txt']);

		adic(adic(:, 1) == 4, 2) = 0;
		adio(adio(:, 1) == 4, 2) = 0;
		adic(adic(:, 1) == 12, 2) = 0;
		adio(adio(:, 1) == 12, 2) = 0;

		vaic(vaic(:, 1) == 4, 2) = 0;
		vaio(vaio(:, 1) == 4, 2) = 0;
		vaic(vaic(:, 1) == 12, 2) = 0;
		vaio(vaio(:, 1) == 12, 2) = 0;

		figure(1)
		plot(adic(:, 1), adic(:, 2), 'or', adio(:, 1), adio(:, 2), '*b'); hold on
		plot(vaic(:, 1), vaic(:, 2), 'gs', vaio(:, 1), vaio(:, 2), 'ko', ...
			'markersize', 8);

		legend('ADI-cuda', 'ADI-omp', 'VAI-cuda', 'VAI-omp');
		title('PL-ADIVAI-1MM');
		fprintf('done\n');
	catch err
		fprintf('error\n');
	end
	hold off
	pause
end

%{
if TEST_VAI_CUDAOMP_CH_1MM
	fprintf('TEST_VAI_CUDAOMP_CH_1MM ... ');
	c = load([dataPath, 'ch_bk_vaicudaiso_lfmfor_1mm_sns.txt']);
	o = load([dataPath, 'ch_bk_vaiompiso_lfmfor_1mm_sns.txt']);
	
	plot(o(:, 1), o(:, 2), 'or', c(:, 1), c(:, 2), '*b');
	fprintf('done\n');
	hold off
	pause
end
%}

%%%%%%%%%
if CH_LFM_TRIP_1MM 
	fprintf('CH_LFM_TRIP_1MM ... ');
	figure(1); clf;
	try
		fid = fopen([dataPath, 'ch_bk_adicudaiso_lfmtriprec_6d_1mm_output/', ...
			'ch_bk_adicudaiso_lfmtriprec_6d_1mm_trip.gs'], 'r');
		adirec = fread(fid, 'double', 'l');
		fclose(fid);

		fid = fopen([dataPath, 'ch_bk_vaicudaiso_lfmtriprec_6d_1mm_output/', ...
			'ch_bk_vaicudaiso_lfmtriprec_6d_1mm_trip.gs'], 'r');
		vairec = fread(fid, 'double', 'l');
		fclose(fid);

		fid = fopen([dataPath, 'ch_bk_adicudaiso_lfmtripfor_6d_1mm_output/', ...
			'ch_bk_adicudaiso_lfmtripfor_6d_1mm_trip.gs'], 'r');
		adifor = fread(fid, 'double', 'l');
		fclose(fid);
	
		fid = fopen([dataPath, 'ch_bk_vaicudaiso_lfmtripfor_6d_1mm_output/', ...
			'ch_bk_vaicudaiso_lfmtripfor_6d_1mm_trip.gs'], 'r');
		vaifor = fread(fid, 'double', 'l');
		fclose(fid);
		
		figure(1)
	
		plot(1:size(adirec, 1), adirec, 'or','markersize', 6); hold on
		plot(1:size(vairec, 1), vairec, '*b');
		plot(1:size(adifor, 1), adifor, 'sg', 'markersize', 8);
		plot(1:size(vaifor, 1), vaifor, 'ok', 'markersize', 12);
		
		legend('ADI.reciprocity', 'VAIiso.reciprocity', ...
			'ADI.forward', 'VAIiso.forward')
		title('CH-LFM-TRIP-1MM');
		fprintf('done\n');
		
	catch err	
		fprintf('error\n');
	end
	hold off
	pause
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% LFM ADI 1mm cuda triple=yes reciprocity=yes
%{
if TEST_LFM_ADIVAI_TRIP_REC_CH_1MM 
	fprintf('TEST_LFM_ADIVAI_TRIP_REC_CH_1MM ... ');
	fid = fopen([dataPath, 'ch_bk_adicudaiso_lfmtriprec_1mm_trip.gs'], 'r');
	g = fread(fid, 'double', 'l');
	g = reshape(g, 60, 41)';
	fclose(fid);

	fid = fopen([dataPath, 'ch_bk_vaicudaiso_lfmtriprec_1mm_trip.gs'], 'r');
	g1 = fread(fid, 'double', 'l');
	g1 = reshape(g1, 60, 41)';
	fclose(fid);

	figure(1)
	for i=1:size(g, 2)
		plot(1:size(g, 1), g(:, i), 'or', 1:size(g1, 1), g1(:, i), '*b');
		legend('ADI.cuda.reciprocity', 'VAIiso.cuda.reciprocity')
		pause(.5)
	end
	
	fprintf('done\n');
	hold off
	pause
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Test reciprocity: LFM ADI 1mm cuda triple=yes reciprocity=NO
if TEST_LFM_ADICUDA_TRIP_RECFOR_CH_1MM 
	fprintf('TEST_LFM_ADICUDA_TRIP_RECFOR_CH_1MM ... ');
	fid = fopen([dataPath, 'ch_bk_adicudaiso_lfmtriprec_1mm_trip.gs'], 'r');
	g = fread(fid, 'double', 'l');
	g = reshape(g, 60, 41)';
	fclose(fid);

	fid = fopen([dataPath, 'ch_bk_adicudaiso_lfmtripfor_1mm_trip.gs'], 'r');
	g1 = fread(fid, 'double', 'l');
	g1 = reshape(g1, 60, 41)';
	fclose(fid);
	figure(1)
	
	for i=1:size(g, 2)
		plot(1:size(g, 1), g(:, i), 'or', 1:size(g1, 1), g1(:, i), '*b');
		legend('ADI.cuda.reciprocity', 'ADI.cuda.forward')
		pause(.5)
	end
	fprintf('done\n');
	hold off
	pause
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
if TEST_LFM_VAICUDA_TRIP_RECFOR_CH_1MM 
	fprintf('TEST_LFM_VAICUDA_TRIP_RECFOR_CH_1MM ... ');
	
	fid = fopen([dataPath, 'ch_bk_vaicudaiso_lfmtriprec_1mm_trip.gs'], 'r');
	g = fread(fid, 'double', 'l');
	g = reshape(g, 60, 41)';

	fid = fopen([dataPath,'ch_bk_vaicudaiso_lfmtripfor_1mm_trip.gs'], 'r');
	g1 = fread(fid, 'double', 'l');
	g1 = reshape(g1, 60, 41)';

	figure(1)
	for i=1:size(g, 2)
		plot(1:size(g, 1), g(:, i), 'or', 1:size(g1, 1), g1(:, i), '*b');
		legend('VAIiso.cuda.reciprocity', 'VAIiso.cuda.forward')
		pause(.5)
	end
	fprintf('done\n');
	hold off
	pause
	
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
if TEST_LFM_ADIVAI_TRIP_RECFOR_CH_1MM
	fprintf('TEST_LFM_ADIVAI_TRIP_RECFOR_CH_1MM ... ');
	
	fid = fopen([dataPath, 'ch_bk_adicudaiso_lfmtriprec_1mm_trip.gs'], 'r');
	g = fread(fid, 'double', 'l');
	g = reshape(g, 60, 41)';
	fclose(fid);

	fid = fopen([dataPath, 'ch_bk_vaicudaiso_lfmtripfor_1mm_trip.gs'], 'r');
	g1 = fread(fid, 'double', 'l');
	g1 = reshape(g1, 60, 41)';
	fclose(fid);

	figure(1)
	for i=1:size(g, 2)
		plot(1:size(g, 1), g(:, i), 'or', 1:size(g1, 1), g1(:, i), '*b');
		legend('ADI.cuda.reciprocity', 'VAIiso.cuda.forward')
		pause(.5)
	end

	fprintf('done\n');
	hold off
	pause;

end
%}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%% conductivity test simplex

if CH_COND_ADI_SIM_2MM
	fprintf('CH_COND_ADI_SIM_2MM ... ');
	figure(1); clf;
	try
		fid = fopen([dataPath, ...
			'ch_bk_adicudaiso_condsim_simulated_2mm_output/', ...
			'ch_simmeas_i117_s58.cio_0'], 'r');
	
		dat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
		fclose(fid);
	
		%[p, n, r, objt, objo, tc1, tc2, tc3, s, oc1, oc2, oc3]
		npoints = size(dat{4}(:), 1);
		
		figure(1)
		subplot(2,2,1)
		plot(1:npoints, dat{4}(1:npoints), 'or')
		legend('Objective Function')
		
		subplot(2, 2, 2)
		plot(1:npoints, dat{6}(1:npoints), 'or'); hold on
		line([0 npoints], [.33 .33])
		legend('WM')
		hold off
	
		subplot(2, 2, 3)
		plot(1:npoints, dat{7}(1:npoints), 'or'); hold on
		line([0 npoints], [0.021 0.021]);
		legend('Skull')
		hold off
	
		subplot(2, 2, 4)
		plot(1:npoints, dat{8}(1:npoints), 'or'); hold on
		line([0 npoints], [0.54 0.54]);
		legend('Scalp')
		hold off
		title('CH-COND-ADI-SIM-2MM')
		fprintf('done \n');
	catch err
		fprintf('error \n');
	end
	
	pause
	hold off
	
end

if CH_COND_ADI_SA_2MM
	fprintf('CH_COND_ADI_SA_2MM ... ');
	figure(1); clf;
	try
		fid = fopen([dataPath, ...
			'ch_bk_adicudaiso_condsa_simulated_2mm_output/', ...
			'ch_simmeas_i117_s58.cio_0'], 'r');
		dat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
		fclose(fid);
	
		%[p, n, r, objt, objo, tc1, tc2, tc3, s, oc1, oc2, oc3]

		figure(1)
		npoints = size(dat{4}(:), 1);
		subplot(2,2,1)
		plot(1:size(dat{4}(1:npoints)), dat{4}(1:npoints), 'or')
		legend('Objective Function')
	
		subplot(2, 2, 2)
		plot(1:npoints, dat{6}(1:npoints), 'or'); hold on
		line([0 npoints*1.1], [.33 .33])
		plot(1:size(dat{6}(1:npoints)), dat{10}(1:npoints), 'og', 'MarkerSize', 2); 
		legend('WM')
		hold off
	
		subplot(2, 2, 3)
		plot(1:npoints, dat{7}(1:npoints), 'or'); hold on
		line([0 npoints*1.1], [0.021 0.021], 'linewidth', 3);
		plot(1:size(dat{6}(1:npoints)), dat{11}(1:npoints), 'og', 'MarkerSize', 2); hold on
		
		legend('Skull')
		hold off
	
		subplot(2, 2, 4)
		plot(1:npoints, dat{8}(1:npoints), 'or'); hold on
		line([0 npoints*1.1], [0.54 0.54]);
		plot(1:size(dat{6}(1:npoints)), dat{12}(1:npoints), 'og', 'MarkerSize', 2); hold on
		
		legend('Scalp')
		
		fprintf('done \n');
	catch err
		fprintf('error \n');
	end
	hold off
	pause
		
end

if CH_COND_VAIISO_SIM_2MM
	fprintf('CH_COND_VAIISO_SIM_2MM ... ');
	figure(1); clf;
	try
		fid = fopen([dataPath, 'ch_bk_vaicudaiso_condsim_simulated_2mm_output/', ...
			'ch_simmeas_i117_s58.cio_0'], 'r');
		dat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
		fclose(fid);
	
		%[p, n, r, objt, objo, tc1, tc2, tc3, s, oc1, oc2, oc3]

		ii = [4, 5, 6, 7, 8];
		figure(1)
		npoints = size(dat{4}(:), 1);
		subplot(2,2,1)
		plot(1:size(dat{4}(1:npoints)), dat{4}(1:npoints), 'or')
		legend('Objective Function')
		
		subplot(2, 2, 2)
		plot(1:size(dat{6}(1:npoints)), dat{6}(1:npoints), 'or'); hold on
		
		line([0 npoints], [.33 .33])
		legend('WM')
		hold off
	
		subplot(2, 2, 3)
		plot(1:npoints, dat{7}(1:npoints), 'or'); hold on
		line([0 npoints], [0.021 0.021]);
		legend('Skull')
		hold off
	
		subplot(2, 2, 4)
		plot(1:npoints, dat{8}(1:npoints), 'or'); hold on
		line([0 npoints], [0.54 0.54]);
		legend('Scalp')
		hold off
		title('CH-COND-VAIISO-SIM-2MM');
		
		fprintf('done \n');
	catch err
		fprintf('error \n');
	end
	hold off
	pause
	
end

if CH_COND_VAIISO_SA_2MM
	fprintf('CH_COND_VAIISO_SA_2MM ... ');
	figure(1); clf;
	try
	fid = fopen([dataPath, 'ch_bk_vaicudaiso_condsa_simulated_2mm_output/', ...
		'ch_simmeas_i117_s58.cio_0'], 'r');
	dat = textscan(fid, '%d %d %d %f %f %f %f %f %s %f %f %f');
	fclose(fid);
	
	%[p, n, r, objt, objo, tc1, tc2, tc3, s, oc1, oc2, oc3] 

	ii = [4, 5, 6, 7, 8];
	npoints = size(dat{4}(:), 1);
	figure(1)
	subplot(2,2,1)
	plot(1:npoints, dat{4}(1:npoints), 'or')
	legend('Objective Function')
	
	
	subplot(2, 2, 2)
	plot(1:size(dat{6}(1:npoints)), dat{6}(1:npoints), 'or'); hold on
	plot(1:size(dat{6}(1:npoints)), dat{10}(1:npoints), 'og', 'MarkerSize', 2); 
	line([0 npoints*1.1], [.33 .33])
	legend('WM')
	hold off
	
	subplot(2, 2, 3)
	plot(1:npoints, dat{7}(1:npoints), 'or'); hold on
	plot(1:size(dat{6}(1:npoints)), dat{11}(1:npoints), 'og', 'MarkerSize', 2); hold on
	line([0 npoints*1.1], [0.021 0.021]);
	legend('Skull')
	hold off
	
	subplot(2, 2, 4)
	plot(1:npoints, dat{8}(1:npoints), 'or'); hold on
	plot(1:size(dat{6}(1:npoints)), dat{12}(1:npoints), 'og', 'MarkerSize', 2); hold on
	line([0 npoints*1.1], [0.54 0.54]);
	legend('Scalp')
	title ('CH-COND-VAIISO-SA-2MM');
	fprintf('done \n');
	catch err
		fprintf('error \n');
	end
	hold off
	pause
end

if CH_COND_VAIANI_SA_2MM
	fprintf('CH_COND_VAIANI_SA_2MM ... ');
	try
		fid = fopen([dataPath, 'ch_bk_vaicudaani_condsa_simulated_2mm_output/', ...
			'ch_simmeas_i117_s58_ani.cio_0'], 'r');
		dat = textscan(fid, '%d %d %d %f %f %f %f %f %f %s %f %f %f %f');
		fclose(fid);
	
		%[p, n, r, objt, objo, tc1, tc2, tc3, s, oc1, oc2, oc3]

		ii = [4, 5, 6, 7, 8];
		npoints = size(dat{4}(:), 1);
		figure(1); clf;

		subplot(2, 2, 1)
		plot(1:npoints, dat{6}(1:npoints), 'or'); hold on
		plot(1:npoints, dat{11}(1:npoints), 'og', 'MarkerSize', 2);
		line([0 npoints], [0.021 0.021])
		
		legend('Skull-iso')
		hold off
	
		subplot(2, 2, 2)
		plot(1:npoints, dat{7}(1:npoints), 'or'); hold on
		plot(1:npoints, dat{12}(1:npoints), 'og', 'MarkerSize', 2);
		line([0 npoints], [0.54 0.54]);
		legend('Scalp')
		hold off
	
		subplot(2, 2, 3)
		plot(1:npoints, dat{8}(1:npoints), 'or'); hold on
		plot(1:npoints, dat{13}(1:npoints), 'og', 'MarkerSize', 2);
		line([0 npoints], [0.021 0.021]);
		legend('Skull-radial')
		hold off
	
		subplot(2, 2, 4)
		plot(1:npoints, dat{9}(1:npoints), 'or'); hold on
		plot(1:npoints, dat{14}(1:npoints), 'og', 'MarkerSize', 2);
		line([0 npoints], [3.0 3.0]);
		legend('Tangential-to-radial')
		title('CH-COND-VAIANI-SA-2MM');
		fprintf('done \n');
	catch err
		fprintf('error \n');
	end
	hold off
	pause
end


if CH_COND_VAIANI_SIM_2MM 
	fprintf('CH_COND_VAIANI_SIM_2MM  ... ');
	try
	fid = fopen([dataPath, 'ch_bk_vaicudaani_condsim_simulated_2mm_output/', ...
		'ch_simmeas_i117_s58_ani.cio_0'], 'r');
	dat = textscan(fid, '%d %d %d %f %f %f %f %f %f %s %f %f %f %f');
	fclose(fid);
	
	%[p, n, r, objt, objo, tc1, tc2, tc3, s, oc1, oc2, oc3] 

	ii = [4, 5, 6, 7, 8];
	npoints = size(dat{4}(:), 1);
	figure(1); clf;

	subplot(2, 2, 1)
	plot(1:npoints, dat{6}(1:npoints), 'or'); hold on
	line([0 npoints], [0.021 0.021])
	legend('Skull-iso')
	hold off
	
	subplot(2, 2, 2)
	plot(1:npoints, dat{7}(1:npoints), 'or'); hold on
	line([0 npoints], [0.54 0.54]);
	legend('Scalp')
	hold off
	
	subplot(2, 2, 3)
	plot(1:npoints, dat{8}(1:npoints), 'or'); hold on
	line([0 npoints], [0.021 0.021]);
	legend('Skull-radial')
	hold off
	
	subplot(2, 2, 4)
	plot(1:npoints, dat{9}(1:npoints), 'or'); hold on
	line([0 npoints], [3.0 3.0]);
	legend('Tangential-to-radial')
	hold off
	fprintf('done \n');
	catch err	
		fprintf('error \n');
	end
	hold off
	pause
	
end










