#!/usr/bin/ruby

# I can't seem to get the json gem properly installed so for the moment
# I'm parsing the JSON myself.
#require 'json' # sudo gem install json
require 'optparse'
require 'open3'


class Arguments
	attr_accessor :verbosity, :onDevice, :maxOpenCards
	def initialize(args)
		super()
		self.verbosity = 0
		self.onDevice = false
		self.maxOpenCards = 3

		opts = OptionParser.new do |opts|
			opts.banner = "A test program to repeatedly open and close browser cards. Will use the\n"
			opts.banner += "Trinity tests URL's in ~/TrinityRunner if present - otherwise will use\n"
			opts.banner += "a few hard-coded url's.\n\n"
			opts.banner += "Usage: #$0 [options]"
			opts.on('-v', '--verbose', 'Display all trunk revision comments in revision.') do
				self.verbosity = 1
			end
			opts.on('-d', '--ondevice', 'Run tests on device (via novacom), otherwise on desktop against simulator (default).') do
				self.onDevice = true
			end
			opts.on('-m', '--maxopen [MAXOPEN]', "The maximum number of open cards (default is #{self.maxOpenCards}).") do |max|
				self.maxOpenCards = Integer(max)
			end
			opts.on_tail('-h', '--help', 'display this help and exit.') do
				puts opts
				exit
			end
		end

		opts.parse!(args)
	end
end

$opts = Arguments.new(ARGV)
$cardCount=0

def openBrowserCard(url)
	cmd = "luna-send -n 1 palm://com.palm.applicationManager/launch '{\"id\":\"com.palm.app.browser\", \"params\":\"{\\\"target\\\":\\\"#{url}\\\"}\"}'"
	if $opts.onDevice then
		# Not working just yet.
		cmd.gsub!(/\\/, '\\\\')
		cmd = "novacom run 'file:///usr/bin/#{cmd}'"
	end
	puts cmd if $opts.verbosity > 0
	pid = nil
	Open3.popen3(cmd) { |stdin, stdout, stderr|

#		response = JSON.parse(line)
#		Parse manually for now...
		while (line = stderr.gets) != nil do
			if line =~ /processId":\s+"(\d+)"/ then
				pid = $1
			end
		end
		while (line = stdout.gets) != nil do
			if line =~ /processId":\s+"(\d+)"/ then
				pid = $1
			end
		end
	}

	$cardCount += 1

	if pid then
		puts "#{$cardCount}=#{url}"
	else
		STDERR.puts "Error opening #{url}"
	end

	return pid
end

def closeCard(pid)
	`luna-send -n 1 palm://com.palm.applicationManager/close {\\"processId\\":\\"#{pid}\\"}`
	if $opts.onDevice then
		cmd = "novacom run 'file:///usr/bin/#{cmd}'"
	end
	puts cmd if $opts.verbosity > 0
end

mapFile = "#{ENV['HOME']}/TrinityRunner/com.palm.automation.endurance/resources/website_url.map" 
if File.exists?(mapFile) then
	urls = Array.new
	File.open(mapFile) { |f|
		while (line = f.gets) != nil do
			line = line.chomp
			next if line =~ /^\s*#/	# Skip comments
			if line =~ /\s*(\S+)\s*/ then
				url = $1
				if not url =~ /^http/ then
					url = 'http://' + url
				end
				urls.push url
			end
		end
	}
else
	puts "#{mapFile} doesn't exist"
	puts "Using hard-coded URL's"
	urls = ['http://www.google.com', 'http://www.cnn.com', 'http://www.yahoo.com']
end

openCards = Array.new
idx = 0
while true
	idx = 0 if idx > urls.length
	openCards.push openBrowserCard(urls[idx])
	sleep 8

	if openCards.length == $opts.maxOpenCards then
		puts "Closing all open cards" if $opts.verbosity > 0
		openCards.each { |pid| closeCard(pid) }
		openCards = Array.new
	end

	idx += 1
end
