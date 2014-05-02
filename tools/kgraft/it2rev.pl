#!/usr/bin/perl -w

my $prefix='';
foreach (@ARGV) {
    if (/^-/) {
    } else {
	$prefix = $_;
    }
}
my %files=();
my $func='';
my $file='';
my $ffref=0;
while(<STDIN>) {
    chomp;
    if (/^U/) {
	#print "ignore $_\n";
    } elsif (/^D (.*):([^: ]*)$/) {
	#print "func $2 in $1\n";
	$func=$2;
	($file = $1) =~ s/^$prefix//;
    } elsif (/^I (.*):([^: ]*)$/) {
	#print "inline $2 in $1\n";
	my $t = $1;
	my $u = $2;
	$t =~ s/^$prefix//;
	$files{$t}->{$u}->{$file}->{$func} = 1;
    }
}
foreach (sort keys %files) {
    foreach my $inlinee (sort keys %{$files{$_}}) {
	print "$_:$inlinee";
	foreach my $ifile (sort keys %{$files{$_}->{$inlinee}}) {
	    foreach my $ifunc (sort keys %{$files{$_}->{$inlinee}->{$ifile}}) {
		print " $ifile:$ifunc";
	    }
	}
	print "\n";
    }
}
