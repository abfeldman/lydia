#!/usr/bin/perl -w

use strict;

# TODO: Timeouts in DAs

my $da = "$ENV{HOME}/develop/lydia/build/solvers/safari/safari";
my @circuits = ( "74182", "74L85" );
my $circuit_dir = "$ENV{HOME}/develop/isc2lydia/weak";
my $obs_dir = "$ENV{HOME}/develop/isc2lydia/obs/weak";
my $circuit_format = "cnf";
my $obs_format = "terms";

sub benchmark_circuit($);

my $circuit;
foreach $circuit (@circuits) {
    benchmark_circuit($circuit);
}

sub benchmark_circuit($)
{
    my $circuit = pop;

    print("circuit: $circuit\n");

    `$da --batch ${circuit_dir}/${circuit}.${circuit_format} ${obs_dir}/${circuit}.${obs_format} ${circuit}.out`
}
