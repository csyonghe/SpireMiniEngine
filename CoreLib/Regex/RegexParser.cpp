#include "RegexTree.h"
#include "../Tokenizer.h"

namespace CoreLib
{
namespace Text
{
	RegexCharSetNode::RegexCharSetNode()
	{
		CharSet = new RegexCharSet();
	}

	RefPtr<RegexNode> RegexParser::Parse(const String &regex)
	{
		src = regex;
		ptr = 0;
		try
		{
			return ParseSelectionNode();
		}
		catch (...)
		{
			return 0;
		}
	}

	RegexNode * RegexParser::ParseSelectionNode()
	{
		if (ptr >= src.Length())
			return 0;
		RefPtr<RegexNode> left = ParseConnectionNode();
		while (ptr < src.Length() && src[ptr] == '|')
		{
			ptr ++;
			RefPtr<RegexNode> right = ParseConnectionNode();
			RegexSelectionNode * rs = new RegexSelectionNode();
			rs->LeftChild = left;
			rs->RightChild = right;
			left = rs;
		}
		return left.Release();
	}

	RegexNode * RegexParser::ParseConnectionNode()
	{
		if (ptr >= src.Length())
		{
			return 0;
		}
		RefPtr<RegexNode> left = ParseRepeatNode();
		while (ptr < src.Length() && src[ptr] != '|' && src[ptr] != ')')
		{
			RefPtr<RegexNode> right = ParseRepeatNode();
			if (right)
			{
				RegexConnectionNode * reg = new RegexConnectionNode();
				reg->LeftChild = left;
				reg->RightChild = right;
				left = reg;
			}
			else
				break;
		}
		return left.Release();
	}

	RegexNode * RegexParser::ParseRepeatNode()
	{
		if (ptr >= src.Length() || src[ptr] == ')' || src[ptr] == '|')
			return 0;
		
		RefPtr<RegexNode> content;
		if (src[ptr] == '(')
		{
			RefPtr<RegexNode> reg;
			ptr ++;
			reg = ParseSelectionNode();
			if (src[ptr] != ')')
			{
				SyntaxError err;
				err.Position = ptr;
				err.Text = "\')\' expected.";
				Errors.Add(err);
				throw 0;
			}
			ptr ++;
			content = reg.Release();
		}
		else
		{
			RefPtr<RegexCharSetNode> reg;
			if (src[ptr] == '[')
			{
				reg = new RegexCharSetNode();
				ptr ++;
				reg->CharSet = ParseCharSet();
				
				if (src[ptr] != ']')
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = "\']\' expected.";
					Errors.Add(err);
					throw 0;
				}
				ptr ++;
			}
			else if (src[ptr] == '\\')
			{
				ptr ++;
				reg = new RegexCharSetNode();
				reg->CharSet = new RegexCharSet();
				switch (src[ptr])
				{
				case '.':
					{
						reg->CharSet->Neg = true;
						break;
					}
				case 'w':
				case 'W':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = 'a';
						range.End = 'z';
						reg->CharSet->Ranges.Add(range);
						range.Begin = 'A';
						range.End = 'Z';
						reg->CharSet->Ranges.Add(range);
						range.Begin = '_';
						range.End = '_';
						reg->CharSet->Ranges.Add(range);
						range.Begin = '0';
						range.End = '9';
						reg->CharSet->Ranges.Add(range);
						if (src[ptr] == 'W')
							reg->CharSet->Neg = true;
						break;
					}
				case 's':
				case 'S':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = ' ';
						range.End = ' ';
						reg->CharSet->Ranges.Add(range);
						range.Begin = '\t';
						range.End = '\t';
						reg->CharSet->Ranges.Add(range);
						range.Begin = '\r';
						range.End = '\r';
						reg->CharSet->Ranges.Add(range);
						range.Begin = '\n';
						range.End = '\n';
						reg->CharSet->Ranges.Add(range);
						if (src[ptr] == 'S')
							reg->CharSet->Neg = true;
						break;
					}
				case 'd':
				case 'D':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = '0';
						range.End = '9';
						reg->CharSet->Ranges.Add(range);
						if (src[ptr] == 'D')
							reg->CharSet->Neg = true;
						break;
					}
				case 'n':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = '\n';
						range.End = '\n';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case 't':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = '\t';
						range.End = '\t';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case 'r':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = '\r';
						range.End = '\r';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case 'v':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = '\v';
						range.End = '\v';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case 'f':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = '\f';
						range.End = '\f';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case '*':
				case '|':
				case '(':
				case ')':
				case '?':
				case '+':
				case '[':
				case ']':
				case '\\':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = src[ptr];
						range.End = range.Begin;
						reg->CharSet->Ranges.Add(range);
						break;
					}
				default:
					{
						SyntaxError err;
						err.Position = ptr;
						err.Text = String("Illegal escape sequence \'\\") + src[ptr] + "\'";
						Errors.Add(err);
						throw 0;
					}
				}
				ptr ++;
			}
			else if (!IsOperator())
			{
				RegexCharSet::RegexCharRange range;
				reg = new RegexCharSetNode();
				reg->CharSet->Neg = false;
				range.Begin = src[ptr];
				range.End = range.Begin;
				ptr ++;
				reg->CharSet->Ranges.Add(range);
			}
			else
			{
				SyntaxError err;
				err.Position = ptr;
				err.Text = String("Unexpected \'") + src[ptr] + '\'';
				Errors.Add(err);
				throw 0;
			}
			content = reg.Release();
		}
		if (ptr < src.Length())
		{
			if (src[ptr] == '*')
			{
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtArbitary;
				ptr ++;
				return node.Release();
			}
			else if (src[ptr] == '?')
			{
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtOptional;
				ptr ++;
				return node.Release();
			}
			else if (src[ptr] == '+')
			{
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtMoreThanOnce;
				ptr ++;
				return node.Release();
			}
			else if (src[ptr] == '{')
			{
				ptr++;
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtSpecified;
				node->MinRepeat = ParseInteger();
				if (src[ptr] == ',')
				{
					ptr ++;
					node->MaxRepeat = ParseInteger();
				}
				else
					node->MaxRepeat = node->MinRepeat;
				if (src[ptr] == '}')
					ptr++;
				else
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = "\'}\' expected.";
					Errors.Add(err);
					throw 0;
				}
				if (node->MinRepeat < 0)
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = "Minimun repeat cannot be less than 0.";
					Errors.Add(err);
					throw 0;
				}
				if (node->MaxRepeat != -1 && node->MaxRepeat < node->MinRepeat)
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = "Max repeat cannot be less than min repeat.";
					Errors.Add(err);
					throw 0;
				}
				return node.Release();
			}
		}
		return content.Release();
	}

	int RegexParser::ParseInteger()
	{
		StringBuilder number;
		while (CoreLib::Text::IsDigit(src[ptr]))
		{
			number.Append(src[ptr]);
			ptr ++;
		}
		if (number.Length() == 0)
			return -1;
		else
			return StringToInt(number.ProduceString());
	}

	bool RegexParser::IsOperator()
	{
		return (src[ptr] == '|' || src[ptr] == '*' || src[ptr] == '(' || src[ptr] == ')'
				|| src[ptr] == '?' || src[ptr] == '+');
	}

	wchar_t RegexParser::ReadNextCharInCharSet()
	{
		if (ptr < src.Length() && src[ptr] != ']')
		{
			if (src[ptr] == '\\')
			{
				ptr ++;
				if (ptr >= src.Length())
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = String("Unexpected end of char-set when looking for escape sequence.");
					Errors.Add(err);
					throw 0;
				}
				wchar_t rs = 0;
				if (src[ptr] == '\\')
					rs = '\\';
				else if (src[ptr] == '^')
					rs = '^';
				else if (src[ptr] == '-')
					rs = '-';
				else if (src[ptr] == ']')
					rs = ']';
				else if (src[ptr] == 'n')
					rs = '\n';
				else if (src[ptr] == 't')
					rs = '\t';
				else if (src[ptr] == 'r')
					rs = '\r';
				else if (src[ptr] == 'v')
					rs = '\v';
				else if (src[ptr] == 'f')
					rs = '\f';
				else
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = String("Illegal escape sequence inside charset definition \'\\") + src[ptr] + "\'";
					Errors.Add(err);
					throw 0;
				}
				ptr ++;
				return rs;
			}
			else
				return src[ptr++];
		}
		else
		{
			SyntaxError err;
			err.Position = ptr;
			err.Text = String("Unexpected end of char-set.");
			Errors.Add(err);
			throw 0;
		}
	}

	RegexCharSet * RegexParser::ParseCharSet()
	{
		RefPtr<RegexCharSet> rs = new RegexCharSet();
		if (src[ptr] == '^')
		{
			rs->Neg = true;
			ptr ++;
		}
		else
			rs->Neg = false;
		RegexCharSet::RegexCharRange range;
		while (ptr < src.Length() && src[ptr] != ']')
		{
			range.Begin = ReadNextCharInCharSet();
			//ptr ++;
			
			if (ptr >= src.Length())
			{
				break;
			}
			if (src[ptr] == '-')
			{
				ptr ++;
				range.End = ReadNextCharInCharSet();	
			}
			else
			{
				range.End = range.Begin;
			}
			rs->Ranges.Add(range);
		
		}
		if (ptr >=src.Length() || src[ptr] != ']')
		{
			SyntaxError err;
			err.Position = ptr;
			err.Text = String("Unexpected end of char-set.");
			Errors.Add(err);
			throw 0;
		}
		return rs.Release();
	}
}
}