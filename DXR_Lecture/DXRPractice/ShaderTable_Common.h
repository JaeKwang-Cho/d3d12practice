#pragma once
#ifdef __INTELLISENSE__
#include <cstring>
#include <cstdint>
#endif

struct PointerWithSize
{
	void* pPointer;
	size_t size;

	PointerWithSize() : pPointer(nullptr), size(0) {}
	PointerWithSize(void* _pPointer, size_t _size) : pPointer(_pPointer), size(_size) {}
};

class ShaderRecord {

public:
	void CopyTo(void* _dest) const {
		uint8_t* byteDest = static_cast<uint8_t*>(_dest);
		memcpy(byteDest, shaderIdentifier.pPointer, shaderIdentifier.size);
		if(localRootArguments.pPointer)
		{
			memcpy(byteDest + shaderIdentifier.size, localRootArguments.pPointer, localRootArguments.size);
		}
	}
public:
	PointerWithSize shaderIdentifier;
	PointerWithSize localRootArguments;

public:
	ShaderRecord(void* _pShaderIdentifier, size_t _shaderIdentifierSize):
		shaderIdentifier(_pShaderIdentifier, _shaderIdentifierSize) 
	{}

	ShaderRecord(void* _pShaderIdentifier, size_t _shaderIdentifierSize, void* _pLocalRootArguments, size_t _localRootArgumentsSize) :
		shaderIdentifier(_pShaderIdentifier, _shaderIdentifierSize),
		localRootArguments(_pLocalRootArguments, _localRootArgumentsSize)
	{}

};