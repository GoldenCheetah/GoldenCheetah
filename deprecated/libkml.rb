require 'formula'

class Libkml < Formula
  homepage 'http://code.google.com/p/libkml/'

  stable do
    url "https://libkml.googlecode.com/files/libkml-1.2.0.tar.gz"
    sha1 "3fa5acdc2b2185d7f0316d205002b7162f079894"

    # Correct an issue where internal third-party libs (libminizip and liburiparser)
    # are installed as dylibs. liburiparser conflicts with uriparser formula.
    # libminizip conflicts with new formula, and some of its symbols have been
    # renamed with prefixes of "libkml_", i.e, can't be linked against for other builds
    # Fix just forces internal libs to be linked statically until the following
    # is addressed upstream: https://code.google.com/p/libkml/issues/detail?id=50
    patch do
      url "https://gist.githubusercontent.com/dakcarto/7419882/raw/10ae08af224b3fee0617fa6288d806d3ccf37c0f/libkml-1.2-static-deps"
      sha1 "eba47421e64e75bcf68026bbbe7c985b3bebcde5"
    end
  end


  head do
    url 'https://github.com/google/libkml.git',
        :revision => "9b50572641f671194e523ad21d0171ea6537426e"

    depends_on "autoconf" => :build
    depends_on "automake" => :build
    depends_on "libtool" => :build
  end

  # Fix compilation with clang and gcc 4.7+
  # https://code.google.com/p/libkml/issues/detail?id=179
  patch :DATA if build.stable?

  def install
    if build.head?
      inreplace "third_party/Makefile.am" do |s|
        s.sub! /(lib_LTLIBRARIES =) libminizip.la liburiparser.la/, "\\1"
        s.sub! /(noinst_LTLIBRARIES = libgtest.la libgtest_main.la)/,
               "\\1 libminizip.la liburiparser.la"
        s.sub! /(libminizip_la_LDFLAGS =)/, "\\1 -static"
        s.sub! /(liburiparser_la_LDFLAGS =)/, "\\1 -static"
      end
      system "./autogen.sh"
    end

    system "./configure", "--prefix=#{prefix}"
    system "make install"
  end
end

__END__
diff --git a/src/kml/base/file_posix.cc b/src/kml/base/file_posix.cc
index 764ae55..8ee9892 100644
--- a/src/kml/base/file_posix.cc
+++ b/src/kml/base/file_posix.cc
@@ -29,6 +29,7 @@
 #include "kml/base/file.h"
 #include <stdlib.h>
 #include <string.h>
+#include <unistd.h>
 #include <sys/types.h>
 #include <sys/stat.h>
