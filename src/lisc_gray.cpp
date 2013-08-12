#include "lisc.hpp"
#include <iostream>


struct module : public ModuleBase
{
	module (Evaluator* ev=nullptr)
		: ModuleBase(ev)
	{
		counter=0;
		REGISTER_METHOD(prim);
		REGISTER_METHOD(sphere);
	}

	double counter;

	Datum prim (const List& form)
	{
		int shape = eval(form.at(1)).list.at(1);
		return makelist("prim_", (double)shape);
	}

	Datum sphere (const List& form)
	{
		counter++;
		return makelist("sphere_", counter);
	}
};

void register_lisc_gray (Evaluator* ev)
{
	new module(ev);
}