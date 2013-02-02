JFLAGS = 

JC = javac

.SUFFIXES: .java .class

.java.class:
	$(JC) $(JFLAGS) $*.java #-Xlint:unchecked

CLASSES = \
	Master.java \
	TestClass.java \
	Worker.java
	
default: classes

classes: $(CLASSES:.java=.class)

clean:
	$(RM) *.class
