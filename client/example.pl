#!/usr/bin/perl
#
# Example brmlife client (originally based on the btraptor).
#
# This client is meant only as an example of implementation of all
# the main features without sophisticated architecture or decision
# making strategies. It should have some basic sustainability, though.
#
# Usage: example.pl [PORT [AGENTID [GENDER]]]
#
# AGENTID may be prefixed with + to indicate newly bred agent.
#
# GENDER may be 0 for no breeding, 1 for "male" (active) or 2 for
# "female" (passive); default is 0; breeding requires the script
# to be run in screen.
#
# To run e.g. 20 instances of this client with breeding enabled,
# start screen and from within run:
#	for i in `seq 1 10`; do screen ./example.pl 27753 0 1; screen ./example.pl 27753 0 2; done

use strict;
use warnings;

# Socket communication should use CR-LF line endings, not just LF.
$/ = "\r\n";

my $remote_port;


# The example agent does most of its decision making in the
# take_action() subroutine. Its policy is to (in this order):
#
# * Breed with encountered matching agents (unless low on energy).
# * Attack any other agents encountered (unless low on energy
#   or just bred - likely to attack newborns).
# * Eat flowers if in immediate vicinity.
# * Roam around semi-aimlessly, trying to look for food.
#
# This agent can be identified based on pheromone #65535.
# Males furthermore secrete pheromone #65534, females #65533.

# The example agent uses $state to hold its state structure. It is
# a HASHREF with the following fields:
# {
# 	agent_id => NUMBER (agent id)
# 	tick => NUMBER (current tick id)
# 	energy => NUMBER (current energy)
# 	visual => [
# 		CHAR[2], ...
# 	] (type-agent character pairs for perceived tiles)
# 	pheromones => [
# 		{ PHID => VALUE, ... }, ...
# 	] (pheromone spectrum for perceived tiles)
#
#	gender => NUMBER (same as GENDER parameter)
#	last_bred => TICKNUMBER (when last bred new agents)
# }


# Read server input associated with a single tick and update the state
# structure accordingly.
sub tick($$) {
	my ($socket, $state) = @_;

	my $line = '';
	print "\n";
	while (chomp($line = <$socket>)) {
		print "# $line\n";
		last if $line eq '';
		if ($line eq 'DEAD')  {
			print "[ii] im dead\n";
			exit(-2);
			return;
		}

		if ($line eq 'BUMP')  {
			print "[ii] bump\n";
			next;
		}

		$line =~ m/^([^ ]+) (.*)$/;
		my ($type, $value) = ($1, $2);

		if ($type eq 'BRED') {
			my ($id, $father_info) = ($value =~ m/^([^ ]+)(?: (.*))?$/);
			my $g = 1 + int rand(2);
			print "[ii] bred $id ($g)\n";
			#open LOG, ">>bred.log"; print LOG "$state->{agent_id} -> $id ($g)\n"; close LOG;
			#system("screen sh -c './$0 $remote_port $id $g; read x'");
			system("screen ./$0 $remote_port $id $g");
			$state->{last_bred} = $value;

		} elsif ($type eq 'agent_id') {
			$value =~ /^\d+$/ or die "[ee] type agent_id wrong value ($value)\n";
			$state->{agent_id} = $value;

		} elsif ($type eq 'tick') {
			$value =~ /^\d+$/ or die "[ee] type tick wrong value ($value)\n";
			$state->{tick} =  $value;

		} elsif ($type eq 'energy') {
			$value =~ /^\d+$/ or die "[ee] type energy wrong value ($value)\n";
			$state->{energy} =  $value;

		} elsif ($type eq 'visual') {
			$value =~ /^([^ ][^ ] )+([^ ][^ ])$/ or die "[ee] type visual wrong value ($value)\n";
			$state->{visual} = [ split(" ", $value) ];

		} elsif ($type eq 'pheromones') {
			$value =~ /^(\((\d+:[0-9.]+,?)*\) ?)+$/ or die "[ee] type pheromones wrong value ($value)\n";
			# Best to read the following from the outside inwards:
			$state->{pheromones} = [
				map { {
					map { split(":", $_) } split(",", $_)
				} } map { y/()//d; $_ } split(" ", $value)
			];
		}
	}
}

# Execute an appropriate action based on the current agent state.
sub take_action($$) {
	my ($socket, $state) = @_;

	# FIXME: We use a common direction choice for both move_dir
	# and attack_dir, but in fact the agent can do both actions
	# in a single tick and they can be in different directions.

	# Relative x,y coordinates for each movement/attack direction.
	my @dirs = ([0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]);

	# Move/attack desires for each direction.
	# We prefer moves in the diagonal direction.
	my @move = (
		(1, 0, 1),
		(0, 0, 0),
		(1, 0, 1));
	my @attack = ( (0, 0, 0), (0, 0, 0), (0, 0, 0));
	my @breed = ( (0, 0, 0), (0, 0, 0), (0, 0, 0));

	# dirindex($x) returns @move, @attack index for given @dirs item.
	sub dirindex { my ($dir) = @_; $dir->[0]+1 + 3*($dir->[1]+1) }

	# Relative x,y coordinates for each visual input, in order.
	my @vdirs = (
		@dirs,
		[0, -2], [1, -2], [2, -2], [2, -1], [2, 0], [2, 1], [2, 2], [1, 2], [0, 2], [-1, 2], [-2, 2], [-2, 1], [-2, 0], [-2, -1], [-2, -2], [-1, -2],
	);

	my $max = $dirs[0];
	# Default direction in case of nothing interesting in the vicinity
	# is [1, -1].

	# In the case of shortage of resources, prefer fleeing to fighting.
	# Also, do not fight when we were just trying to breed - this is
	# poor man's way to avoid killing our newborns.
	my $flee = ($state->{energy} < 10000 or $state->{tick} - $state->{last_bred} < 5);

	# Examine our neighborhood and adjust preference for each direction
	# based on what we sense.
	for my $i (0..$#{$state->{visual}}) {
		my ($type, $agent) = split(//, $state->{visual}->[$i]);
		my $ph = $state->{pheromones}->[$i];
		my $dir = $vdirs[$i];

		if (abs($dir->[0]) > 1 or abs($dir->[1]) > 1) {
			# We do not support processing visual information
			# for inputs two tiles away.
			next;
		}

		if ($agent eq 'x') {
			# Herp
			if ($state->{energy} < 18000) {
				$move[dirindex($dir)] += 2;
			} else {
				$move[dirindex($dir)] -= 2;
			}
		}

		if ($agent eq 'a') {
			# Corpse
			if ($state->{energy} < 17000) {
				$move[dirindex($dir)] += 4;
			} else {
				$move[dirindex($dir)] -= 4;
			}
		}

		if ($agent eq 'A') {
			# Agent
			# For breeding, we need to be sure we are breeding
			# with female agent, which is a bit tricky.
			my $breeding_target = ($state->{gender} == 1
				and defined $ph->{65533}
				and $ph->{65533} > 5
				and (not defined $ph->{65534} or $ph->{65533} > $ph->{65534})
				and $ph->{65535} / $ph->{65533} < 1.5);
			if ($breeding_target) {
				$move[dirindex($dir)] += 6;
				$breed[dirindex($dir)] += 1;
			} elsif (not $flee) {
				$move[dirindex($dir)] += 7;
				$attack[dirindex($dir)] += 1;
			} else {
				$move[dirindex($dir)] -= 7;
			}
		}

		if ($move[dirindex($dir)] > $move[dirindex($max)] or
		    ($move[dirindex($dir)] == $move[dirindex($max)] and int rand(2))) {
			$max = $dir;
		}
	}

	# Debug print
	print "moves ".join(", ", @move)." => (".dirindex($max).":$max->[0],$max->[1])\n";

	# Execute actions!
	if ($attack[dirindex($max)] > 0) {
		print $socket $state->{tick}." attack_dir $max->[0] $max->[1] ".($state->{energy}/5)."\r\n";
		print $state->{tick}." attack_dir $max->[0] $max->[1] ".($state->{energy}/5)."\r\n";
	} elsif ($breed[dirindex($max)] > 0) {
		print $socket $state->{tick}." breed_dir $max->[0] $max->[1]\r\n";
		print $state->{tick}." breed_dir $max->[0] $max->[1]\r\n";
		$state->{last_bred} = $state->{tick};
	} else {
		print $socket $state->{tick}." move_dir $max->[0] $max->[1]\r\n";
		print $state->{tick}." move_dir $max->[0] $max->[1]\r\n";
	}
	# We unconditionally secrete this pheromone for identification
	# by others of our kin.
	print $socket $state->{tick}." secrete 65535 1\r\n";
	print $state->{tick}." secrete 65535 1\r\n";
	if ($state->{gender} == 1) {
		print $socket $state->{tick}." secrete 65534 1\r\n";
		print $state->{tick}." secrete 65534 1\r\n";
	} elsif ($state->{gender} == 2) {
		print $socket $state->{tick}." secrete 65533 1\r\n";
		print $state->{tick}." secrete 65533 1\r\n";
	}
	print $socket "\r\n";
}


# Connect

my ($remote_host, $socket);
$remote_host = "localhost";
$remote_port = $ARGV[0];
$remote_port ||= 27753;

use IO::Socket;
$socket = IO::Socket::INET->new(
    PeerAddr => $remote_host,
    PeerPort => $remote_port,
    Proto    => "tcp",
    Type     => SOCK_STREAM
) or die "Couldn't connect to $remote_host:$remote_port : $@\n";
print "[ii] connected\r\n";


# Parse parameters

my $agentid = $ARGV[1];
$agentid ||= 0;
my $bred = $agentid =~ s/^\+//;
my $gender = $ARGV[2];
$gender ||= 0;


# Negotiate attributes

if ($agentid) {
	print "[ii] recovering agent $ARGV[1]\r\n";
	print $socket "agent_id $ARGV[1]\r\n";
}

if ($bred or not $agentid) {
	# Agent attributes - the default values:
	print $socket "move 1.0\r\n";
	print $socket "attack 0.5\r\n";
	print $socket "defense 0.5\r\n";
	my $base_key = 10838479;
	if ($gender == 1) {
		print $socket "breeding_key1 $base_key\r\n";
		print $socket "breeding_key2 -$base_key\r\n";
	} else {
		print $socket "breeding_key1 -$base_key\r\n";
		print $socket "breeding_key2 $base_key\r\n";
	}
}
print $socket "\r\n";
print "[ii] agent created\r\n";


# Start tick loop

my $state = { gender => $gender, last_bred => 0 };
while (1) {
	tick($socket, $state);
	# Debug print
	print $state->{agent_id} . " " .$state->{energy} . "\n";
	print "visual [", join('], [', @{$state->{visual}}), "]\n";
	# The following could be written more succintly, but it is here
	# to show possible iteration over pheromones:
	print "pherom ";
	for my $i (0..$#{$state->{pheromones}}) {
		print "[";
		for my $pi (keys %{$state->{pheromones}->[$i]}) {
			print "$pi=>".$state->{pheromones}->[$i]->{$pi}." ";
		}
		print "] ";
	}
	print "\n";

	take_action($socket, $state);
}

shutdown($socket, 2);
