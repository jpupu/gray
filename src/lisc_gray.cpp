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
		List shape = eval(form.at(1));
		int shapenum = shape.at(1);
		return makelist("prim_", (double)shapenum);
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