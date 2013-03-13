all:
	+ cd src; make $(MFLAGS); cd ..

i686:
	+ cd src ; make i686 $(MFLAGS); cd ..

amd64:
	+ cd src; make amd64 $(MFLAGS); cd ..

arm:
	+ cd src; make arm $(MFLAGS); cd ..

clean:
	cd src ; $(MAKE) clean ; cd ..
	rm -rf build/
