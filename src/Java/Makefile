JFLAGS = 

JC = javac

.SUFFIXES: .java .class

.java.class:
	$(JC) $(JFLAGS) $*.java #-Xlint:unchecked

CLASSES = \
	SystemCall.java \
	TestClass.java \
	BarkBeetlePresto.java \
	Master.java \
	Worker.java
	
default: classes

classes: $(CLASSES:.java=.class)

clean:
	$(RM) *.class
