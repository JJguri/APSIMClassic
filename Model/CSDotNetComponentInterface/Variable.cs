﻿using System;
using System.Text;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Runtime.InteropServices;
using ModelFramework;
using CSGeneral;
using CMPServices;

namespace ModelFramework
{
    public class Variable : NamedItem
    {
        /// --------------------------------------------------------------------------
        /// This class encapsulates an APSIM variable.
        /// --------------------------------------------------------------------------
        private ApsimComponent Component;
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="component"></param>
        /// <param name="VariableName">FQN of the variable</param>
        public Variable(object component, String VariableName)
        {
            Component = component as ApsimComponent;
            Name = VariableName;
        }

        public bool Exists()
        {
            WrapBuiltInVariable<String> Data = new WrapBuiltInVariable<String>();
            return Component.Get(Name, Data, true);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public bool ToBoolean()
        {
            WrapBuiltInVariable<bool> Data = new WrapBuiltInVariable<bool>();
            Component.Get(Name, Data, false);
            return Data.Value;
        }
        public int ToInt32()
        {
            WrapBuiltInVariable<int> Data = new WrapBuiltInVariable<int>();
            Component.Get(Name, Data, false);
            return Data.Value;
        }
        public float ToSingle()
        {
            WrapBuiltInVariable<float> Data = new WrapBuiltInVariable<float>();
            Component.Get(Name, Data, false);
            return Data.Value;
        }
        public double ToDouble()
        {
            WrapBuiltInVariable<double> Data = new WrapBuiltInVariable<double>();
            Component.Get(Name, Data, false);
            return Data.Value;
        }
        public override String ToString()
        {
            WrapBuiltInVariable<String> Data = new WrapBuiltInVariable<String>();
            Component.Get(Name, Data, false);
            return Data.Value;
        }
        public DateTime ToDateTime()
        {
            WrapBuiltInVariable<int> Data = new WrapBuiltInVariable<int>();
            Component.Get(Name, Data, false);
            int JulianDate = Data.Value;
            return DateUtility.JulianDayNumberToDateTime(JulianDate);
        }
        public int[] ToInt32Array()
        {
            WrapBuiltInVariable<int[]> Data = new WrapBuiltInVariable<int[]>();
            Component.Get(Name, Data, false);
            return Data.Value;
        }
        public float[] ToSingleArray()
        {
            WrapBuiltInVariable<float[]> Data = new WrapBuiltInVariable<float[]>();
            Component.Get(Name, Data, false);
            return Data.Value;
        }
        public double[] ToDoubleArray()
        {
            WrapBuiltInVariable<double[]> Data = new WrapBuiltInVariable<double[]>();
            Component.Get(Name, Data, false);
            return Data.Value;
        }
        public String[] ToStringArray()
        {
            WrapBuiltInVariable<String[]> Data = new WrapBuiltInVariable<String[]>();
            Component.Get(Name, Data, false);
            return Data.Value;
        }

        public Boolean Set(bool Value)
        {
            WrapBuiltInVariable<bool> Data = new WrapBuiltInVariable<bool>();
            Data.Value = Value;
            return Component.Set(Name, Data);
        }
        public Boolean Set(int Value)
        {
            WrapBuiltInVariable<int> Data = new WrapBuiltInVariable<int>();
            Data.Value = Value;
            return Component.Set(Name, Data);
        }
        public Boolean Set(float Value)
        {
            WrapBuiltInVariable<float> Data = new WrapBuiltInVariable<float>();
            Data.Value = Value;
            return Component.Set(Name, Data);
        }
        public Boolean Set(double Value)
        {
            WrapBuiltInVariable<double> Data = new WrapBuiltInVariable<double>();
            Data.Value = Value;
            return Component.Set(Name, Data);
        }
        public Boolean Set(String Value)
        {
            WrapBuiltInVariable<String> Data = new WrapBuiltInVariable<String>();
            Data.Value = Value;
            return Component.Set(Name, Data);
        }

        public Boolean Set(bool[] Values)
        {
            WrapBuiltInVariable<bool[]> Data = new WrapBuiltInVariable<bool[]>();
            Data.Value = Values;
            return Component.Set(Name, Data);
        }
        public Boolean Set(int[] Values)
        {
            WrapBuiltInVariable<int[]> Data = new WrapBuiltInVariable<int[]>();
            Data.Value = Values;
            return Component.Set(Name, Data);
        }
        public Boolean Set(float[] Values)
        {
            WrapBuiltInVariable<float[]> Data = new WrapBuiltInVariable<float[]>();
            Data.Value = Values;
            return Component.Set(Name, Data);
        }
        public Boolean Set(double[] Values)
        {
            WrapBuiltInVariable<double[]> Data = new WrapBuiltInVariable<double[]>();
            Data.Value = Values;
            return Component.Set(Name, Data);
        }
        public Boolean Set(String[] Values)
        {
            WrapBuiltInVariable<String[]> Data = new WrapBuiltInVariable<String[]>();
            Data.Value = Values;
            return Component.Set(Name, Data);
        }
    }

    // --------------------------------------------------------------------
    // This class wraps a FactoryProperty and a built in type (e.g. Single, 
    // Double etc). It then makes it look like an ApsimType with pack,
    // unpack methods etc.
    // --------------------------------------------------------------------
    public class WrapBuiltInVariable<T> : TypeInterpreter, ApsimType
    {
        /*This class is a quick way to wrap the TTypedValue into a generic class
            * that handles scalars and arrays of scalars - NH
            I think this class should be superceded by TypeInterpreter or TDDMLValue
            */
        protected Type tType;
        public T Value;
        //protected override TDDMLValue DDMLValue; */
        public WrapBuiltInVariable()
        {
            tType = typeof(T);
            // DDMLValue = new TDDMLValue(DDML(), "");
        }
        public override void pack(out byte[] messageData)
        {
            setValue(Value);
            messageData = new byte[DDMLValue.sizeBytes()];
            DDMLValue.getData(ref messageData);
        }
        public override void unpack(TTypedValue src)
        {
            DDMLValue.setValue(src);
            setVariableValue();
        }
        public override void unpack(byte[] messageData)
        {
            //::unpackWithConverter(messageData, Value);
            DDMLValue.setData(messageData, messageData.Length, 0);
            setVariableValue();
        }
        /// <summary>
        /// Set the internal Value from the DDMLValue that was set in the unpack()
        /// </summary>
        private void setVariableValue()
        {
            if (tType == typeof(Boolean))
            {
                Value = (T)(Convert.ChangeType(DDMLValue.asBool(), typeof(T)));
            }
            else if (tType == typeof(Int32))
            {
                Value = (T)(Convert.ChangeType(DDMLValue.asInt(), typeof(T)));
            }
            else if (tType == typeof(Single))
            {
                Value = (T)(Convert.ChangeType(DDMLValue.asSingle(), typeof(T)));
            }
            else if (tType == typeof(double))
            {
                Value = (T)(Convert.ChangeType(DDMLValue.asDouble(), typeof(T)));
            }
            else if (tType == typeof(String))
            {
                Value = (T)(Convert.ChangeType(DDMLValue.asStr(), typeof(T)));
            }
            else if (tType == typeof(Boolean[]))
            {
                Value = (T)(Convert.ChangeType(DDMLValue.asBooleanArray(), typeof(T)));
            }
            else if (tType == typeof(Int32[]))
            {
                Value = (T)(Convert.ChangeType(DDMLValue.asIntArray(), typeof(T)));
            }
            else if (tType == typeof(Single[]))
            {
                Value = (T)(Convert.ChangeType(DDMLValue.asSingleArray(), typeof(T)));
            }
            else if (tType == typeof(double[]))
            {
                Value = (T)(Convert.ChangeType(DDMLValue.asDoubleArray(), typeof(T)));
            }
            else if (tType == typeof(String[]))
            {
                Value = (T)(Convert.ChangeType(DDMLValue.asStringArray(), typeof(T)));
            }
            else if (tType == typeof(DateTime))
            {
                double JulianDate = DDMLValue.asDouble();               //stored as a double
                DateTime jDate = TTimeValue.JDToDateTime(JulianDate);
                Value = (T)(Convert.ChangeType(jDate, typeof(T)));
            }
        }
        public override uint memorySize()
        {
            setValue(Value);    //ensure the DDMLValue is updated
            return DDMLValue.sizeBytes();
        }
        public override String DDML()
        {
            return DDML(Value);
        }

        public String DDML(T Value)
        {
            String result = "<type/>";
            if (typeof(T) == typeof(Boolean))
            {
                result = "<type kind=\"boolean\"/>";
            }
            else if (typeof(T) == typeof(Int32))
            {
                result = "<type kind=\"integer4\"/>";
            }
            else if (typeof(T) == typeof(Single))
            {
                result = "<type kind=\"single\"/>";
            }
            else if (typeof(T) == typeof(double))
            {
                result = "<type kind=\"double\"/>";
            }
            else if (typeof(T) == typeof(String))
            {
                result = "<type kind=\"string\"/>";
            }
            else if (typeof(T) == typeof(Boolean[]))
            {
                result = "<type kind=\"boolean\" array=\"T\"/>";
            }
            else if (typeof(T) == typeof(Int32[]))
            {
                result = "<type kind=\"integer4\" array=\"T\"/>";
            }
            else if (typeof(T) == typeof(Single[]))
            {
                result = "<type kind=\"single\" array=\"T\"/>";
            }
            else if (typeof(T) == typeof(double[]))
            {
                result = "<type kind=\"double\" array=\"T\"/>";
            }
            else if (typeof(T) == typeof(String[]))
            {
                result = "<type kind=\"string\" array=\"T\"/>";
            }
            else if (typeof(T) == typeof(DateTime))
            {
                result = "<type kind=\"double\"/>";
            }
            return result;
        }
        public void setValue(object value)
        {
            Value = (T)Convert.ChangeType(value, tType);

            if (tType == typeof(Boolean))
            {
                DDMLValue.setValue(Convert.ToBoolean(value));
            }
            else if (tType == typeof(Int32))
            {
                DDMLValue.setValue(Convert.ToInt32(value));
            }
            else if (tType == typeof(Single))
            {
                DDMLValue.setValue(Convert.ToSingle(value));
            }
            else if (tType == typeof(double))
            {
                DDMLValue.setValue(Convert.ToDouble(value));
            }
            else if (tType == typeof(String))
            {
                DDMLValue.setValue(Convert.ToString(value));
            }
            else if (tType == typeof(Boolean[]))
            {
                DDMLValue.setValue((Boolean[])Convert.ChangeType(value, typeof(Boolean[])));
            }
            else if (tType == typeof(Int32[]))
            {
                DDMLValue.setValue((Int32[])Convert.ChangeType(value, typeof(Int32[])));
            }
            else if (tType == typeof(Single[]))
            {
                DDMLValue.setValue((Single[])Convert.ChangeType(value, typeof(Single[])));
            }
            else if (tType == typeof(Double[]))
            {
                DDMLValue.setValue((Double[])Convert.ChangeType(value, typeof(Double[])));
            }
            else if (tType == typeof(String[]))
            {
                DDMLValue.setValue((String[])Convert.ChangeType(value, typeof(String[])));
            }
            else if (tType == typeof(DateTime))
            {
                double JulianDate = DateUtility.DateTimeToJulianDayNumber((DateTime)Convert.ChangeType(value, typeof(DateTime))); 
                DDMLValue.setValue(JulianDate);
            }
        }
    }

}
    
