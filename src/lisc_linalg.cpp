#include "lisc_linalg.hpp"

LiscLinAlg::LiscLinAlg (Evaluator* ev)
	: ModuleBase(ev)
{
	REGISTER_METHOD(vec3);
	REGISTER_METHOD(identity);
	REGISTER_METHOD(translate);
}

Datum LiscLinAlg::vec3 (const List& form)
{
	if (form.size() != 4) {
		throw std::runtime_error("vec3 must have 3 elements");
	}
	glm::vec3 v;
	for (int i = 1; i < 4; i++) {
		v[i-1] = eval(form.at(i));
	}
	vec3s.push_back(v);
	return makelist("vec3_", vec3s.size()-1);
}

Datum LiscLinAlg::identity (const List& form)
{
	if (form.size() != 1) {
		throw std::runtime_error("identity does not take parameters");
	}
	xforms.push_back(Transform());
	return makelist("xform_", xforms.size()-1);
}

Datum LiscLinAlg::translate (const List& form)
{
	if (form.size() != 4) {
		throw std::runtime_error("translate takes 3 parameters");
	}
	glm::vec3 v;
	for (int i = 1; i < 4; i++) {
		v[i-1] = eval(form.at(i));
	}
	xforms.push_back(Transform::translate(v));
	return makelist("xform_", xforms.size()-1);
}


// struct module
// {
// 	static vec3 getvec (Evaluator* ev, const List& form)
// 	{
// 		vec3 v;
// 		for (int k = 0; k < 3; k++) {
// 			v[k] = ev->evaluate(form[i])
// 			a[k] += ev->evaluate(form[i].list[k+1]).get_number();
// 		}
// 	}


// 	static Datum vec(Evaluator* ev, const List& form)
// 	{
// 		if (form.size() != 4) {
// 			throw std::runtime_error("vec must have 3 elements");
// 		}
// 		List r = { Datum("vec") };
// 		for (int i = 1; i < 4; i++) {
// 			Datum it = ev->evaluate(form[i]);
// 			if (!it.is_number()) {
// 				throw std::runtime_error("vec elements must evaluate to numbers");
// 			}
// 			r.push_back(it);
// 		}
// 		return Datum(r);
// 	}
	
// 	static Datum vadd(Evaluator* ev, const List& form)
// 	{
// 		double a[3] = {0,0,0};
// 		for (size_t i = 1; i < form.size(); ++i) {
// 			if (!form[i].is_function("vec3")) throw std::runtime_error("add operands must be vec3's");

// 			for (int k = 0; k < 3; k++) {
// 				a[k] += ev->evaluate(form[i].list[k+1]).get_number();
// 			}
// 		}
// 		return Datum(makelist("vec3", a[0], a[1], a[2]));
// 	}
	

// };

// void register_lisc_linalg (Evaluator& ev)
// {
// 	ev.register_function("vec", module::vec);
// 	ev.register_function("v+", module::vadd);
// }