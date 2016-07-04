system buffer_sa0(bool o, i)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = i);
	!h => !o;
}

system inverter_sa0(bool o, i)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !i);
	!h => !o;
}

system xor2_sa0(bool o, i1, i2)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 != i2));
	!h => !o;
}

system and2_sa0(bool o, i1, i2)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2));
	!h => !o;
}

system and3_sa0(bool o, i1, i2, i3)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3));
	!h => !o;
}

system and4_sa0(bool o, i1, i2, i3, i4)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4));
	!h => !o;
}

system and5_sa0(bool o, i1, i2, i3, i4, i5)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4 and i5));
	!h => !o;
}

system and6_sa0(bool o, i1, i2, i3, i4, i5, i6)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4 and i5 and i6));
	!h => !o;
}

system and7_sa0(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4 and i5 and i6 and i7));
	!h => !o;
}

system and8_sa0(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8));
	!h => !o;
}

system and9_sa0(bool o, i1, i2, i3, i4, i5, i6, i7, i8, i9)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8 and i9));
	!h => !o;
}

system nand2_sa0(bool o, i1, i2)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2));
	!h => !o;
}

system nand3_sa0(bool o, i1, i2, i3)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3));
	!h => !o;
}

system nand4_sa0(bool o, i1, i2, i3, i4)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3 and i4));
	!h => !o;
}

system nand5_sa0(bool o, i1, i2, i3, i4, i5)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3 and i4 and i5));
	!h => !o;
}

system nand6_sa0(bool o, i1, i2, i3, i4, i5, i6)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3 and i4 and i5 and i6));
	!h => !o;
}

system nand7_sa0(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3 and i4 and i5 and i6 and i7));
	!h => !o;
}

system nand8_sa0(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8));
	!h => !o;
}

system or2_sa0(bool o, i1, i2)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2));
	!h => !o;
}

system or3_sa0(bool o, i1, i2, i3)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3));
	!h => !o;
}

system or4_sa0(bool o, i1, i2, i3, i4)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3 or i4));
	!h => !o;
}

system or5_sa0(bool o, i1, i2, i3, i4, i5)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3 or i4 or i5));
	!h => !o;
}

system or6_sa0(bool o, i1, i2, i3, i4, i5, i6)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3 or i4 or i5 or i6));
	!h => !o;
}

system or7_sa0(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3 or i4 or i5 or i6 or i7));
	!h => !o;
}

system or8_sa0(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8));
	!h => !o;
}

system nor2_sa0(bool o, i1, i2)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2));
	!h => !o;
}

system nor3_sa0(bool o, i1, i2, i3)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3));
	!h => !o;
}

system nor4_sa0(bool o, i1, i2, i3, i4)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4));
	!h => !o;
}

system nor5_sa0(bool o, i1, i2, i3, i4, i5)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4 or i5));
	!h => !o;
}

system nor6_sa0(bool o, i1, i2, i3, i4, i5, i6)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4 or i5 or i6));
	!h => !o;
}

system nor7_sa0(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7));
	!h => !o;
}

system nor8_sa0(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8));
	!h => !o;
}

system nor9_sa0(bool o, i1, i2, i3, i4, i5, i6, i7, i8, i9)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8 or i9));
	!h => !o;
}

system halfadder2_sa0(bool i1, i2, sum, carry)
{
	system and2_sa0 A(carry, i1, i2);
	system xor2_sa0 X(sum, i1, i2);
}

system fulladder2_sa0(bool i1, i2, ci, sum, carry)
{
	bool f, p, q;

	system halfadder2_sa0 HA1(i1, i2, f, p);
	system halfadder2_sa0 HA2(ci, f, sum, q);
	system or2_sa0 O(carry, p, q);
}

system buffer_sa1(bool o, i)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = i);
	!h => o;
}

system inverter_sa1(bool o, i)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !i);
	!h => o;
}

system xor2_sa1(bool o, i1, i2)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 != i2));
	!h => o;
}

system and2_sa1(bool o, i1, i2)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2));
	!h => o;
}

system and3_sa1(bool o, i1, i2, i3)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3));
	!h => o;
}

system and4_sa1(bool o, i1, i2, i3, i4)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4));
	!h => o;
}

system and5_sa1(bool o, i1, i2, i3, i4, i5)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4 and i5));
	!h => o;
}

system and6_sa1(bool o, i1, i2, i3, i4, i5, i6)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4 and i5 and i6));
	!h => o;
}

system and7_sa1(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4 and i5 and i6 and i7));
	!h => o;
}

system and8_sa1(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8));
	!h => o;
}

system and9_sa1(bool o, i1, i2, i3, i4, i5, i6, i7, i8, i9)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8 and i9));
	!h => o;
}

system nand2_sa1(bool o, i1, i2)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2));
	!h => o;
}

system nand3_sa1(bool o, i1, i2, i3)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3));
	!h => o;
}

system nand4_sa1(bool o, i1, i2, i3, i4)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3 and i4));
	!h => o;
}

system nand5_sa1(bool o, i1, i2, i3, i4, i5)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3 and i4 and i5));
	!h => o;
}

system nand6_sa1(bool o, i1, i2, i3, i4, i5, i6)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3 and i4 and i5 and i6));
	!h => o;
}

system nand7_sa1(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3 and i4 and i5 and i6 and i7));
	!h => o;
}

system nand8_sa1(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 and i2 and i3 and i4 and i5 and i6 and i7 and i8));
	!h => o;
}

system or2_sa1(bool o, i1, i2)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2));
	!h => o;
}

system or3_sa1(bool o, i1, i2, i3)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3));
	!h => o;
}

system or4_sa1(bool o, i1, i2, i3, i4)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3 or i4));
	!h => o;
}

system or5_sa1(bool o, i1, i2, i3, i4, i5)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3 or i4 or i5));
	!h => o;
}

system or6_sa1(bool o, i1, i2, i3, i4, i5, i6)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3 or i4 or i5 or i6));
	!h => o;
}

system or7_sa1(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3 or i4 or i5 or i6 or i7));
	!h => o;
}

system or8_sa1(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = (i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8));
	!h => o;
}

system nor2_sa1(bool o, i1, i2)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2));
	!h => o;
}

system nor3_sa1(bool o, i1, i2, i3)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3));
	!h => o;
}

system nor4_sa1(bool o, i1, i2, i3, i4)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4));
	!h => o;
}

system nor5_sa1(bool o, i1, i2, i3, i4, i5)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4 or i5));
	!h => o;
}

system nor6_sa1(bool o, i1, i2, i3, i4, i5, i6)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4 or i5 or i6));
	!h => o;
}

system nor7_sa1(bool o, i1, i2, i3, i4, i5, i6, i7)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7));
	!h => o;
}

system nor8_sa1(bool o, i1, i2, i3, i4, i5, i6, i7, i8)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8));
	!h => o;
}

system nor9_sa1(bool o, i1, i2, i3, i4, i5, i6, i7, i8, i9)
{
	bool h;

	attribute probability(h) = (h ? 0.99 : 0.01);
	attribute health(h) = h;
	
	h => (o = !(i1 or i2 or i3 or i4 or i5 or i6 or i7 or i8 or i9));
	!h => o;
}

system halfadder2_sa1(bool i1, i2, sum, carry)
{
	system and2_sa1 A(carry, i1, i2);
	system xor2_sa1 X(sum, i1, i2);
}

system fulladder2_sa1(bool i1, i2, ci, sum, carry)
{
	bool f, p, q;

	system halfadder2_sa1 HA1(i1, i2, f, p);
	system halfadder2_sa1 HA2(ci, f, sum, q);
	system or2_sa1 O(carry, p, q);
}