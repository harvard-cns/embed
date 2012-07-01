main()
{
  char strdat[80]="ltest.dat";
  int nolin=0, iestat=0;

  pprn1_(&nolin,strdat,&iestat);
  printf("exit= %d\n",iestat);
  exit(-1);
}

/* dummy routine */
void funobj()
{
}
