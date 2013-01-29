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
$content =~ /<tr class="racenet_row rankedmaps_click" >(.*?)<\/tr>/s;
if (!defined $1) {
    print "No highscore found\n";
    exit;
}
my $entry = $1;
$entry =~ /(\d+:\d{2}.\d{3})/;
my $time = $1;
$entry =~ /<a href="\/player\/index\/id\/\d+\/">(.*?)<\/a>/;
my $name = $1;
$name =~ s/<\/?span.*?>//g;
$name =~ s/&lt;/</g;
$name =~ s/&gt;/>/g;
print "Racenet record: $time by $name\n";
exit;

sub query {
    my($page) = @_;
    return get('http://www.warsow-race.net' . $page);
}
