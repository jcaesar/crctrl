#!/usr/bin/perl -w

use Net::IRC;
use strict;


my $irc = new Net::IRC;


my $conn = $irc->newconn(
	Server 		=> shift || 'irc.euirc.net',
	Port		=> shift || '6667', 
	Nick		=> 'CServ',
	Ircname		=> '',
	Username	=> 'cserv'
);


$conn->{channel} = shift || '#cserv';

sub on_connect {

	my $conn = shift;
  

	$conn->join($conn->{channel});
	$conn->privmsg($conn->{channel}, 'Moin.');
	$conn->{connected} = 1;

}

sub on_join {


	my ($conn, $event) = @_;

	# this is the nick that just joined
	my $nick = $event->{nick};
	# say hello to the nick in public
	if($nick ne 'CServ') {
		$conn->privmsg($conn->{channel}, "Moin, $nick!");
	}
}
	
sub on_part {
	
	my ($conn, $event) = @_;

	my $nick = $event->{nick};
	if($nick ne 'CServ') {
		$conn->privmsg($conn->{channel}, "Bye, $nick!");
	}
}


$conn->add_handler('join', \&on_join);
$conn->add_handler('part', \&on_part);


$conn->add_handler('376', \&on_connect);


$irc->start();

