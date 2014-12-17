#!/usr/bin/perl

sub printClass
{
  printf( OUTPUT $_);
  while(<INPUT>)
  {
    printf( OUTPUT $_);
    if(/{.*}\s*;/)
    {
      #Do not stop at:  enum x { a,b,c };
    }
    else
    {
      if(/}\s*;/)
        { last;}
    }
  };
}

#####################################################################################
# Extracts VDr Setup Class Definitions from <vdr-dir>/menu.c
#####################################################################################
my $file   = $ARGV[0];
my $output = "vdrsetupclasses.h";

printf("FILE=$file\n");

open(INPUT,  $file)      or die "Can't open $file ";
open(OUTPUT, ">$output") or die "Can't open $file ";

 printf( OUTPUT "/***********************************************************\n");
 printf( OUTPUT " * Header file holding Class Definitions from <VDR>/menu.c *\n");
 printf( OUTPUT " *                                                         *\n");
 printf( OUTPUT " * This file is generated automaticly!                     *\n");
 printf( OUTPUT " *                                                         *\n");
 printf( OUTPUT " ***********************************************************/\n\n");

 printf( OUTPUT "#ifndef VDRSETUPCLASSES_H \n");
 printf( OUTPUT "#define VDRSETUPCLASSES_H \n\n");
 
 printf( OUTPUT "#include \"vdr/diseqc.h\"\n");
 printf( OUTPUT "#if VDRVERSNUM >= 10716\n");
 printf( OUTPUT "#include \"vdr/sourceparams.h\"\n");
 printf( OUTPUT "#endif\n\n");

 while (<INPUT>) 
 {   
    if (/^\s*class\s+\w+MenuSetupBase/) 
    {
       printClass;
       printf(OUTPUT "\n\n");
    }
    if (/^\s*class\s+\w+\s*:\s*public\s+cMenuSetupBase/) 
    {
       printClass;
       printf(OUTPUT "\n\n");
    }
    #class * : public cOsdMenu {
    if (/^\s*class\s+\w+\s*:\s*public\s+cOsdMenu/) 
    {
       printf( OUTPUT "#if VDRVERSNUM >= 10716\n");
       printClass;
       printf( OUTPUT "#endif\n\n");
    }
 }

printf( OUTPUT "#endif // VDRSETUPCLASSES_H \n");
close(OUTPUT);
close(FILE)


