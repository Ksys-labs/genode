CC=gcc
CXX=g++
CXXFLAGS= -O2 -g -Wall -I../../include
SOURCES= generate-keys.cc signfile.cc
PREFIX?=
PWD:=$(shell pwd)

all: generate-keys signfile

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

ecc.o:
	$(CC) $(CXXFLAGS) -I../../contrib/micro-ecc -c ../../contrib/micro-ecc/ecc.c -o $@

sha2.o:
	$(CC) $(CXXFLAGS) -I../../contrib/sha2-1.0.1 -c ../../contrib/sha2-1.0.1/sha2.c -o $@

generate-keys: ecc.o generate-keys.o
	$(CXX) $(CXXFLAGS) ecc.o generate-keys.o -o $@

signfile: ecc.o signfile.o sha2.o
	$(CXX) $(CXXFLAGS) ecc.o sha2.o signfile.o -o $@

install:
	ln -s $(PWD)/generate-keys $(PREFIX)/
	ln -s $(PWD)/signfile $(PREFIX)/
	ln -s $(PWD)/signfile.py $(PREFIX)/

clean:
	rm -rf generate-keys
	rm -rf signfile
	rm -rf *.o
