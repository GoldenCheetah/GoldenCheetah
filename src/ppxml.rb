#!/usr/bin/env ruby
require 'rexml/document'
xml = REXML::Document.new(STDIN)
xml.write(STDOUT, 2)
