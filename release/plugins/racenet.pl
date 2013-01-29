#!/usr/bin/perl

use strict;
use warnings;

use LWP::Simple;

my $content = query('/maps/index/order/playtime/dir/DESC/filter/' . $ARGV[0]);
$content =~ /<a class="wmtt" .*? href="(.*?)">$ARGV[0]<\/a>/;
if (!defined $1) {
    print "Map not found\n";
    exit;
}
$content = query($1);
my @entries = $content =~ /<tr class="racenet_row rankedmaps_click" >(.*?)<\/tr>/sg;
if (!defined $1) {
    print "No highscore found\n";
    exit;
}
my $nopj_done = 0;
my $pj_done = 0;
for my $entry(@entries) {
    my $pj = $entry =~ /color: red/;
    if (($pj && $pj_done) || (!$pj && $nopj_done)) {
        next;
    }
    $entry =~ /(\d+:\d{2}.\d{3})/;
    my $time = $1;
    $entry =~ /<a href="\/player\/index\/id\/\d+\/">(.*?)<\/a>/;
    my $name = $1;
    $name =~ s/<\/?span.*?>//g;
    $name =~ s/&lt;/</g;
    $name =~ s/&gt;/>/g;
    print "Racenet " . ($pj ? "PJ" : "NOPJ") . " record: $time by $name\n";
    if ($pj) {
        $pj_done = 1;
    } else {
        $nopj_done = 1;
    }
}
exit;

sub query {
    my($page) = @_;
    return get('http://www.warsow-race.net' . $page);
}
