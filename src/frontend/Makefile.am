AM_CPPFLAGS = $(CXX17_FLAGS) $(SSL_CFLAGS) -I$(srcdir)/../util
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = example

example_SOURCES = example.cc
example_LDADD = ../util/libgldemoutil.a $(GLU_LIBS) $(GLEW_LIBS) $(GLFW3_LIBS)
