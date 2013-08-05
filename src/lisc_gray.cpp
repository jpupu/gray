#include "lisc.hpp"
#include <iostream>
struct module
{
	module (Evaluator* ev=nullptr)
		: ev(ev)
	{
		counter=0;
		ev->register_function("prim", [this](Evaluator* ev,const List& form)->Datum
			{this->ev=ev;return this->prim(form);});
		ev->register_function("sphere", [this](Evaluator* ev,const List& form)->Datum
			{this->ev=ev;return this->sphere(form);});
	}

	Evaluator* ev;
	double counter;

	Datum prim (const List& form)
	{
		int shape = ev->evaluate(form.at(1)).list.at(1).get_number();
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