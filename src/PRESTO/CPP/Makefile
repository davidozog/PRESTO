all:
	g++ -g -c -o obj/CE_Adaptor.o CE_Adaptor.cpp 
	g++ -g -c -o obj/Matlab_Adaptor.o Matlab_Adaptor.cpp 
	g++ -g -c -o obj/Comp_Model.o Comp_Model.cpp 
	g++ -g -c -o obj/Master_Worker_Model.o Master_Worker_Model.cpp 
	mpicxx -g -o PRESTO obj/CE_Adaptor.o obj/Matlab_Adaptor.o obj/Comp_Model.o obj/Master_Worker_Model.o PRESTO_main.cpp 

clean:
	rm obj/*.o PRESTO
