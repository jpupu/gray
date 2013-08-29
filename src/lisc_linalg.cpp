#include "lisc_linalg.hpp"

LiscLinAlg::LiscLinAlg (Evaluator* ev)
	: ModuleBase(ev)
{
	REGISTER_METHOD(vec3);
	REGISTER_METHOD(identity);
	REGISTER_METHOD(translate);
	REGISTER_METHOD(scale);
	REGISTER_METHOD(rotate);
	REGISTER_METHOD_NAME("t*", tmul);
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

Datum LiscLinAlg::scale (const List& form)
{
	if (form.size() != 2 && form.size() != 4) {
		throw std::runtime_error("scale takes 1 or 3 parameters");
	}
	glm::vec3 v;
	if (form.size() == 2) {
		v = glm::vec3((float)eval(form.at(1)));
	}
	else {
		for (int i = 1; i < 4; i++) {
			v[i-1] = eval(form.at(i));
		}
	}
	xforms.push_back(Transform::scale(v));
	return makelist("xform_", xforms.size()-1);
}

Datum LiscLinAlg::rotate (const List& form)
{
	if (form.size() != 5) {
		throw std::runtime_error("rotate takes 4 parameters");
	}
	glm::vec3 v;
	float angle = eval(form.at(1));
	for (int i = 0; i < 3; i++) {
		v[i] = eval(form.at(i+2));
	}
	xforms.push_back(Transform::rotate(angle, v));
	return makelist("xform_", xforms.size()-1);
}



Datum LiscLinAlg::tmul (const List& form)
{
	if (form.size() < 2) {
		throw std::runtime_error("tmul takes >=1 parameters");
	}

	Transform R;

	for (size_t i = 1; i < form.size(); i++) {
		List l = eval(form.at(i));
		if ((std::string)l.at(0) != "xform_") {
			throw std::runtime_error("tmul takes transform parameters");
		}
		Transform M = xforms.at(l.at(1));
		R = Transform(R.m * M.m, M.m_inv * R.m_inv);
	}

	xforms.push_back(R);
	return makelist("xform_", xforms.size()-1);
}
