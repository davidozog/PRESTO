all:
	g++ -g -c -o objects/CE_Adaptor.o CE_Adaptor.cpp 
	g++ -g -c -o objects/Matlab_Adaptor.o Matlab_Adaptor.cpp 
	g++ -g -c -o objects/Comp_Model.o Comp_Model.cpp 
	g++ -g -c -o objects/Master_Worker_Model.o Master_Worker_Model.cpp 
	mpicxx -g -o FRAMPTON objects/CE_Adaptor.o objects/Matlab_Adaptor.o objects/Comp_Model.o objects/Master_Worker_Model.o FRAMPTON_main.cpp 

clean:
	rm objects/* FRAMPTON
