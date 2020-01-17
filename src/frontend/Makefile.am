AM_CPPFLAGS = $(CXX17_FLAGS) $(SSL_CFLAGS) -I/usr/include -I$(srcdir)/../util
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = example

example_SOURCES = example.cc
example_LDADD = -L/usr/lib -leyelink_core_graphics -leyelink_core -lpthread ../util/libgldemoutil.a $(GLU_LIBS) $(GLEW_LIBS) $(GLFW3_LIBS) 
