AUTOMAKE_OPTIONS=foreign

ehlibdir = $(pkglibdir)/codecs
ehlib_LTLIBRARIES = libpim_ex.la
libpim_ex_la_CXXFLAGS = $(AM_CXXFLAGS)
libpim_ex_la_LDFLAGS = -export-dynamic -shared
libpim_ex_la_SOURCES = pim.cc


ehlib_LTLIBRARIES += libtoken_ring_ex.la
libtoken_ring_ex_la_CXXFLAGS = $(AM_CXXFLAGS)
libtoken_ring_ex_la_LDFLAGS = -export-dynamic -shared
libtoken_ring_ex_la_SOURCES = token_ring.cc

AM_CXXFLAGS = @AM_CXXFLAGS@
