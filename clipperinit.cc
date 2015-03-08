#include <node.h>
#include <nan.h>

#include "src/clipper.hpp"
#include "src/clipper.cpp"

#define DEFAULT_SCALE 10000

using namespace node;
using namespace v8;
using namespace ClipperLib;

NAN_METHOD(Simplify) {
	NanScope();

	const int scaleFactor = (args.Length() > 1) ? ((args[1]->IsNumber()) ? v8::Number::Cast(*args[1])->Value() : DEFAULT_SCALE ) : DEFAULT_SCALE;

    // Check to make sure that the first argument is an array of even length
	if (args[0]->IsArray() && args[0].As<Array>()->Length() % 2 == 0) {
		try {
			// Cast our first argument as an array
			Local<Array> points = args[0].As<Array>();

			ClipperLib::Polygon polygon;
			ClipperLib::Polygons polysout;

			// Construct the shape using the array of points
			for (int i = 0, limiti = points->Length(); i < limiti; i += 2) {
				Local<Value> pairA = points->Get(i);
				Local<Value> pairB = points->Get(i+1);
				// Push point onto polygon with scale factor
				polygon.push_back(
					IntPoint(
						(int)(pairA->NumberValue() * scaleFactor),
						(int)(pairB->NumberValue() * scaleFactor)
					)
				);
			}

			// Clean polygon
			CleanPolygon(polygon, polygon); 

			// Simplify polygon
			SimplifyPolygon(polygon, polysout, pftNonZero);
			
			// Get the resultant simplified polygons
			Local<Object> obj = NanNew<Object>();
			// Create array containers for Outer Polygons and Inner Polygons (holes)
			Local<Array> outPolygons = NanNew<Array>();
			Local<Array> inPolygons = NanNew<Array>();
			for (std::vector<ClipperLib::Polygon>::iterator polyiter = polysout.begin(); polyiter != polysout.end(); ++polyiter) {
				// For each point in the polygon
				Handle<Array> points = NanNew<Array>();
				for (std::vector<IntPoint>::iterator iter = polyiter->begin(); iter != polyiter->end(); ++iter) {
					// Retrieve the points and undo scale
					Local<Value> x = NanNew<Number>((double)iter->X / scaleFactor);
					Local<Value> y = NanNew<Number>((double)iter->Y / scaleFactor);
					points->Set(points->Length(), x);
					points->Set(points->Length(), y);
				}

				// If the orientation of polygon returns true, it is an outer polygon
				if (Orientation(*polyiter)) outPolygons->Set(outPolygons->Length(), points);
				// Otherwise it is an inner polygon (a hole)
				else inPolygons->Set(inPolygons->Length(), points);
			}

			// Set in/out properties for return object
			obj->Set(NanNew<String>("out"), outPolygons);
			obj->Set(NanNew<String>("in"), inPolygons);

			NanReturnValue(obj);
			return;
		} catch (...) {
			NanReturnValue(NanFalse());
			return;
		}
	}

	NanReturnValue(NanFalse());
}


NAN_METHOD(Union) {
	NanScope();

	// const int scaleFactor = (args.Length() > 2) ? ((args[2]->IsNumber()) ? v8::Number::Cast(*args[2])->Value() : DEFAULT_SCALE ) : DEFAULT_SCALE;
	const int scaleFactor = DEFAULT_SCALE;
    
	try {
		ClipperLib::Polygon* polysin = new ClipperLib::Polygon[args.Length()];

		// Union
		Clipper c;

		// Construct the shape using the array of points
		for (int j = 0; j < args.Length(); j++) {
			// Check to make sure that the first argument is an array of even length
			if (!args[0]->IsArray() || args[0].As<Array>()->Length() % 2 != 0) {
				NanReturnValue(NanFalse());
				return;
			}

			Local<Array> points = args[j].As<Array>();
			for (int i = 0, limiti = points->Length(); i < limiti; i += 2) {
				Local<Value> pairA = points->Get(i);
				Local<Value> pairB = points->Get(i+1);
				// Push point onto polygon with scale factor
				polysin[j].push_back(
					IntPoint(
						(int)(pairA->NumberValue() * scaleFactor),
						(int)(pairB->NumberValue() * scaleFactor)
					)
				);
			}
			CleanPolygon(polysin[j], polysin[j]);
			c.AddPolygon(polysin[j], ptClip);
		}

		ClipperLib::Polygons polysout;
		c.Execute(ctUnion, polysout, pftEvenOdd, pftPositive);
		
		// Get the resultant simplified polygons
		Local<Object> obj = NanNew<Object>();
		// Create array containers for Outer Polygons and Inner Polygons (holes)
		Handle<Array> outPolygons = NanNew<Array>();
		Handle<Array> inPolygons = NanNew<Array>();
		for (std::vector<ClipperLib::Polygon>::iterator polyiter = polysout.begin(); polyiter != polysout.end(); ++polyiter) {
			// For each point in the polygon
			Handle<Array> points = NanNew<Array>();
			for (std::vector<IntPoint>::iterator iter = polyiter->begin(); iter != polyiter->end(); ++iter) {
				// Retrieve the points and undo scale
				Local<Value> x = NanNew<Number>((double)iter->X / scaleFactor);
				Local<Value> y = NanNew<Number>((double)iter->Y / scaleFactor);
				points->Set(points->Length(), x);
				points->Set(points->Length(), y);
			}

			// If the orientation of polygon returns true, it is an outer polygon
			if (Orientation(*polyiter)) outPolygons->Set(outPolygons->Length(), points);
			// Otherwise it is an inner polygon (a hole)
			else inPolygons->Set(inPolygons->Length(), points);
		}

		// Set in/out properties for return object
		obj->Set(NanNew<String>("out"), outPolygons);
		obj->Set(NanNew<String>("in"), inPolygons);

		NanReturnValue(obj);
		return;
	} catch (...) {
		NanReturnValue(NanFalse());
		return;
	}

	NanReturnValue(NanFalse());
}

void InitAll(Handle<Object> exports) {
  exports->Set(NanNew<String>("simplify"),
    NanNew<FunctionTemplate>(Simplify)->GetFunction());

  exports->Set(NanNew<String>("union"),
    NanNew<FunctionTemplate>(Union)->GetFunction());
}

NODE_MODULE(addon, InitAll)
