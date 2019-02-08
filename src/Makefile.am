AM_CFLAGS = $(VARNISHAPI_CFLAGS) -Wall -Werror -lrabbitmq
AM_LDFLAGS = $(VARNISHAPI_LIBS) $(VMOD_LDFLAGS) 

vmod_LTLIBRARIES = libvmod_rmq.la

libvmod_rmq_la_SOURCES = \
	vmod_rmq.c

nodist_libvmod_rmq_la_SOURCES = \
	vcc_rmq_if.c \
	vcc_rmq_if.h

dist_man_MANS = vmod_rmq.3

@BUILD_VMOD_RMQ@

AM_TESTS_ENVIRONMENT = \
	PATH="$(VMOD_TEST_PATH)" \
	LD_LIBRARY_PATH="$(VARNISH_LIBRARY_PATH)"
TEST_EXTENSIONS = .vtc
VTC_LOG_COMPILER = varnishtest -v
AM_VTC_LOG_FLAGS = -Dvmod_rmq="$(VMOD_RMQ)"

TESTS = @VMOD_TESTS@

EXTRA_DIST = \
	vmod_rmq.vcc \
	$(VMOD_TESTS)
