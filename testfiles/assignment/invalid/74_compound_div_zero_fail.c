int main()
{
  int a;
  a = 5;
  a /= 0; // division par zéro (constante) - should be detected/warned or fail
  return 0;
}
