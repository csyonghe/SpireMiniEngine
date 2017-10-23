#include "stdafx.h"
#include "CppUnitTest.h"
#include "../CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace CoreLib;
using namespace VectorMath;

namespace UnitTest
{
	TEST_CLASS(VectorMathTest)
	{
	public:
		TEST_METHOD(RotAxisAngle0)
		{
			Matrix4 rotation;
			Matrix4::Rotation(rotation, Vec3::Create(1.0f, 0.0f, 0.0f), Math::Pi * 0.5f);
			Vec3 v = Vec3::Create(0.0f, 0.0f, -1.0f);
			Vec3 v_rs = rotation.TransformNormal(v);
			Assert::IsTrue((Vec3::Create(0.0f, 1.0f, 0.0f) - v_rs).Length() < 1e-5f);
		}

		TEST_METHOD(RotAxisAngle1)
		{
			Matrix4 rotation;
			Matrix4::Rotation(rotation, Vec3::Create(0.0f, 1.0f, 0.0f), Math::Pi * 0.5f);
			Vec3 v = Vec3::Create(0.0f, 0.0f, -1.0f);
			Vec3 v_rs = rotation.TransformNormal(v);
			Assert::IsTrue((Vec3::Create(-1.0f, 0.0f, 0.0f) - v_rs).Length() < 1e-5f);
		}

		TEST_METHOD(RotAxisAngle2)
		{
			Matrix4 rotation;
			Matrix4::Rotation(rotation, Vec3::Create(0.0f, 0.0f, 1.0f), Math::Pi * 0.5f);
			Vec3 v = Vec3::Create(1.0f, 0.0f, 0.0f);
			Vec3 v_rs = rotation.TransformNormal(v);
			Assert::IsTrue((Vec3::Create(0.0f, 1.0f, 0.0f) - v_rs).Length() < 1e-5f);
		}

		TEST_METHOD(RotAxisAngle3)
		{
			Matrix4 rotation;
			Matrix4::RotationX(rotation, Math::Pi * 0.5f);
			Vec3 v = Vec3::Create(0.0f, 0.0f, -1.0f);
			Vec3 v_rs = rotation.TransformNormal(v);
			Assert::IsTrue((Vec3::Create(0.0f, 1.0f, 0.0f) - v_rs).Length() < 1e-5f);
		}

		TEST_METHOD(RotAxisAngle4)
		{
			Matrix4 rotation;
			Matrix4::RotationY(rotation, Math::Pi * 0.5f);
			Vec3 v = Vec3::Create(0.0f, 0.0f, -1.0f);
			Vec3 v_rs = rotation.TransformNormal(v);
			Assert::IsTrue((Vec3::Create(-1.0f, 0.0f, 0.0f) - v_rs).Length() < 1e-5f);
		}

		TEST_METHOD(RotAxisAngle5)
		{
			Matrix4 rotation;
			Matrix4::RotationZ(rotation, Math::Pi * 0.5f);
			Vec3 v = Vec3::Create(1.0f, 0.0f, 0.0f);
			Vec3 v_rs = rotation.TransformNormal(v);
			Assert::IsTrue((Vec3::Create(0.0f, 1.0f, 0.0f) - v_rs).Length() < 1e-5f);
		}

		TEST_METHOD(MatrixToEuler)
		{
			Matrix4 rotationz, rotationy;
			Matrix4::RotationZ(rotationz, Math::Pi * 0.4f);
			Matrix4::RotationY(rotationy, Math::Pi * 0.3f);
			Matrix4 m;
			Matrix4::Multiply(m, rotationz, rotationy);
			float x, y, z;
			MatrixToEulerAngle(m.GetMatrix3(), x, y, z, EulerAngleOrder::YXZ);
			Assert::IsTrue(Math::Max(abs(y-Math::Pi*0.3f), abs(z-Math::Pi*0.4f)) < 1e-5f);
		}

		TEST_METHOD(MatrixToEuler1)
		{
			Matrix4 rotationx, rotationy, rotationz;
			Matrix4::RotationX(rotationx, Math::Pi * 0.4f);
			Matrix4::RotationY(rotationy, Math::Pi * 0.3f);
			Matrix4::RotationZ(rotationz, -Math::Pi * 0.7f);
			Matrix4 m;
			Matrix4::Multiply(m, rotationz, rotationy);
			Matrix4::Multiply(m, rotationx, m);
			float x, y, z;
			MatrixToEulerAngle(m.GetMatrix3(), x, y, z, EulerAngleOrder::YZX);
			Quaternion q1;
			Quaternion q;
			q = Quaternion::FromMatrix(m.GetMatrix3());
			EulerAngleToQuaternion(q1, x, y, z, EulerAngleOrder::YZX);
			Assert::IsTrue((q-q1).Length() < 1e-5f);
		}
		TEST_METHOD(MatrixToEuler2)
		{
			Matrix4 rotationx, rotationy, rotationz;
			Matrix4::RotationX(rotationx, Math::Pi * 0.4f);
			Matrix4::RotationY(rotationy, Math::Pi * 0.3f);
			Matrix4::RotationZ(rotationz, -Math::Pi * 0.7f);
			Matrix4 m;
			Matrix4::Multiply(m, rotationx, rotationy);
			Matrix4::Multiply(m, rotationz, m);
			float x, y, z;
			MatrixToEulerAngle(m.GetMatrix3(), x, y, z, EulerAngleOrder::YXZ);
			Quaternion q1;
			Quaternion q;
			q = Quaternion::FromMatrix(m.GetMatrix3());
			EulerAngleToQuaternion(q1, x, y, z, EulerAngleOrder::YXZ);
			Assert::IsTrue((q - q1).Length() < 1e-5f);
		}
		TEST_METHOD(MatrixToEuler3)
		{
			Matrix4 rotationx, rotationy, rotationz;
			Matrix4::RotationX(rotationx, Math::Pi * 0.4f);
			Matrix4::RotationY(rotationy, Math::Pi * 0.3f);
			Matrix4::RotationZ(rotationz, -Math::Pi * 0.7f);
			Matrix4 m;
			Matrix4::Multiply(m, rotationx, rotationz);
			Matrix4::Multiply(m, rotationy, m);
			float x, y, z;
			MatrixToEulerAngle(m.GetMatrix3(), x, y, z, EulerAngleOrder::ZXY);
			Quaternion q1;
			Quaternion q;
			q = Quaternion::FromMatrix(m.GetMatrix3());
			EulerAngleToQuaternion(q1, x, y, z, EulerAngleOrder::ZXY);
			Assert::IsTrue((q - q1).Length() < 1e-5f);
		}
	};
}