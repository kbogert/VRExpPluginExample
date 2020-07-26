#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"

#include "RenderUtils.h"
#include "GeomTools.h"
#include "DrawDebugHelpers.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Net/Core/PushModel/PushModel.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"
//#include "ImageWrapper/Public/IImageWrapper.h"
//#include "ImageWrapper/Public/IImageWrapperModule.h"

#include "VRRenderTargetManager.generated.h"

class UVRRenderTargetManager;

// #TODO: Dirty rects so don't have to send entire texture?

namespace RLE_Funcs
{
	enum RLE_Flags
	{
		RLE_CompressedByte = 1,
		RLE_CompressedShort = 2,
		RLE_Compressed24 = 3,
		RLE_NotCompressedByte = 4,
		RLE_NotCompressedShort = 5,
		RLE_NotCompressed24 = 6,
		//RLE_Empty = 3,
		//RLE_AllSame = 4,
		RLE_ContinueRunByte = 7,
		RLE_ContinueRunShort = 8,
		RLE_ContinueRun24 = 9
	};

	template <typename DataType>
	static bool RLEEncodeLine(TArray<DataType>* LineToEncode, TArray<uint8>* EncodedLine);

	template <typename DataType>
	static bool RLEEncodeBuffer(DataType* BufferToEncode, uint32 EncodeLength, TArray<uint8>* EncodedLine);

	template <typename DataType>
	static void RLEDecodeLine(TArray<uint8>* LineToDecode, TArray<DataType>* DecodedLine, bool bCompressed);

	template <typename DataType>
	static void RLEDecodeLine(const uint8* LineToDecode, uint32 Num, TArray<DataType>* DecodedLine, bool bCompressed);

	static inline void RLEWriteContinueFlag(uint32 Count, uint8** loc);

	template <typename DataType>
	static inline void RLEWriteRunFlag(uint32 Count, uint8** loc, TArray<DataType>& Data, bool bCompressed);
}

USTRUCT(BlueprintType, Category = "VRExpansionLibrary")
struct VREXPANSIONPLUGIN_API FBPVRReplicatedTextureStore
{
	GENERATED_BODY()
public:

	// Not automatically replicated, we are skipping it so that the array isn't checked
	// We manually copy the data into the serialization buffer during netserialize and keep
	// a flip flop dirty flag

	UPROPERTY()
		TArray<uint8> PackedData;
	TArray<uint16> UnpackedData;

	UPROPERTY()
	uint32 Width;

	UPROPERTY()
	uint32 Height;

	UPROPERTY()
		bool bUseColorMap;

	UPROPERTY()
		bool bIsZipped;

	EPixelFormat PixelFormat;

	void Reset()
	{
		PackedData.Reset();
		UnpackedData.Reset();
		Width = 0;
		Height = 0;
		PixelFormat = (EPixelFormat)0;
		bUseColorMap = false;
		bIsZipped = false;
	}

	void PackData()
	{
		if (UnpackedData.Num() > 0)
		{

			/*IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
			TSharedPtr<IImageWrapper> imageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

			imageWrapper->SetRaw(UnpackedData.GetData(), UnpackedData.Num(), Width, Height, ERGBFormat::RGBA, 8);
			const TArray64<uint8>& ImgData = imageWrapper->GetCompressed(1);


			PackedData.Reset(ImgData.Num());
			PackedData.AddUninitialized(ImgData.Num());
			FMemory::Memcpy(PackedData.GetData(), ImgData.GetData(), ImgData.Num());*/

			TArray<uint8> TmpPacked;
			RLE_Funcs::RLEEncodeBuffer<uint16>(UnpackedData.GetData(), UnpackedData.Num(), &TmpPacked);
			UnpackedData.Reset();

			if (TmpPacked.Num() > 512)
			{	
				FArchiveSaveCompressedProxy Compressor(PackedData, NAME_Zlib, COMPRESS_BiasSpeed);
				Compressor << TmpPacked;
				Compressor.Flush();
				bIsZipped = true;
			}
			else
			{
				PackedData = TmpPacked;
				bIsZipped = false;
			}
		}
	}


	void UnPackData()
	{
		if (PackedData.Num() > 0)
		{
			/*IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
			TSharedPtr<IImageWrapper> imageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
			

			if (imageWrapper.IsValid() && (PackedData.Num() > 0) && imageWrapper->SetCompressed(PackedData.GetData(), PackedData.Num()))
			{
				Width = imageWrapper->GetWidth();
				Height = imageWrapper->GetHeight();

				if (imageWrapper->GetRaw(ERGBFormat::BGRA, 8, UnpackedData))
				{
					//bSucceeded = true;
				}
			}*/

			if (bIsZipped)
			{
				TArray<uint8> RLEEncodedData;
				FArchiveLoadCompressedProxy DataArchive(PackedData, NAME_Zlib);
				DataArchive << RLEEncodedData;
				RLE_Funcs::RLEDecodeLine<uint16>(&RLEEncodedData, &UnpackedData, true);
			}
			else
			{
				RLE_Funcs::RLEDecodeLine<uint16>(&PackedData, &UnpackedData, true);
			}
			
			PackedData.Reset();
		}
	}


	/** Network serialization */
	// Doing a custom NetSerialize here because this is sent via RPCs and should change on every update
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = true;

		Ar.SerializeBits(&bUseColorMap, 1);
		Ar.SerializeBits(&bIsZipped, 1);
		Ar.SerializeIntPacked(Width);
		Ar.SerializeIntPacked(Height);
		Ar.SerializeBits(&PixelFormat, 8);

		// Serialize the raw transaction

		//Ar.SerializeIntPacked(UncompressedBufferSize);

		Ar << PackedData;

		uint32 UncompressedBufferSize = PackedData.Num();
		/*if (UncompressedBufferSize > 0)
		{
			Ar.SerializeCompressed(PackedData.GetData(), UncompressedBufferSize, NAME_Zlib, ECompressionFlags::COMPRESS_BiasSpeed);
		}*/

		return bOutSuccess;
	}

};

template<>
struct TStructOpsTypeTraits< FBPVRReplicatedTextureStore > : public TStructOpsTypeTraitsBase2<FBPVRReplicatedTextureStore>
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true,
	};
};


USTRUCT()
struct FRenderDataStore {
	GENERATED_BODY()

	TArray<FColor> ColorData;
	FRenderCommandFence RenderFence;
	FIntPoint Size2D;
	EPixelFormat PixelFormat;

	FRenderDataStore() {
	}
};

/**
* This class is used as a proxy to send owner only RPCs
*/
UCLASS(ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API ARenderTargetReplicationProxy : public AActor
{
	GENERATED_BODY()

public:
	ARenderTargetReplicationProxy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(Replicated)
		TWeakObjectPtr<UVRRenderTargetManager> OwningManager;

	FBPVRReplicatedTextureStore TextureStore;
	
	UPROPERTY(Transient)
		int32 BlobNum;

	void SendInitMessage();

	UFUNCTION()
	void SendNextDataBlob();

	FTimerHandle SendTimer_Handle;

	// Maximum size of texture blobs to use for sending (size of chunks that it gets broken down into)
	UPROPERTY()
		int32 TextureBlobSize;

	// Maximum bytes per second to send, you will want to play around with this and the
	// MaxClientRate settings in config in order to balance the bandwidth and avoid saturation
	// If you raise this above the max replication size of a 65k byte size then you will need
	// To adjust the max size in engine network settings.
	UPROPERTY()
		int32 MaxBytesPerSecondRate;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
	{
		if(SendTimer_Handle.IsValid())
			GetWorld()->GetTimerManager().ClearTimer(SendTimer_Handle);

		Super::EndPlay(EndPlayReason);
	}


	UFUNCTION(Reliable, Client, WithValidation)
		void InitTextureSend(int32 Width, int32 Height, int32 TotalDataCount, int32 BlobCount, EPixelFormat PixelFormat, bool bIsZipped);

	UFUNCTION(Reliable, Server, WithValidation)
		void Ack_InitTextureSend(int32 TotalDataCount);

	UFUNCTION(Reliable, Client, WithValidation)
		void ReceiveTextureBlob(const TArray<uint8>& TextureBlob, int32 LocationInData, int32 BlobCount);

	UFUNCTION(Reliable, Server, WithValidation)
		void Ack_ReceiveTextureBlob(int32 BlobCount);

	UFUNCTION(Reliable, Client, WithValidation)
		void ReceiveTexture(const FBPVRReplicatedTextureStore&TextureData);

};



USTRUCT()
struct FClientRepData {
	GENERATED_BODY()

	UPROPERTY()
		TWeakObjectPtr<APlayerController> PC;

	UPROPERTY()
		TWeakObjectPtr<ARenderTargetReplicationProxy> ReplicationProxy;

	UPROPERTY()
		bool bIsRelevant;

	UPROPERTY()
		bool bIsDirty;

	FClientRepData() 
	{
		bIsRelevant = false;
		bIsDirty = false;
	}
};


/**
* This class stores reading requests for rendertargets and iterates over them
* It returns the pixel data at the end of processing
* It references code from: https://github.com/TimmHess/UnrealImageCapture
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRRenderTargetManager : public UActorComponent
{
	GENERATED_BODY()

public:

    UVRRenderTargetManager(const FObjectInitializer& ObjectInitializer);
	bool bIsStoringImage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RenderTargetManager")
		bool bInitiallyReplicateTexture;

	// Maximum size of texture blobs to use for sending (size of chunks that it gets broken down into)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RenderTargetManager")
		int32 TextureBlobSize;

	// Maximum bytes per second to send, you will want to play around with this and the
	// MaxClientRate settings in config in order to balance the bandwidth and avoid saturation
	// If you raise this above the max replication size of a 65k byte size then you will need
	// To adjust the max size in engine network settings.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RenderTargetManager")
		int32 MaxBytesPerSecondRate;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RenderTargetManager")
		UCanvasRenderTarget2D* RenderTarget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RenderTargetManager")
		int32 RenderTargetWidth;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RenderTargetManager")
		int32 RenderTargetHeight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RenderTargetManager")
		FColor ClearColor;

	// List of 16 colors that we allow to draw on the render target with
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RenderTargetManager|ColorMap")
		TMap<FColor, uint8> ColorMap;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RenderTargetManager|ColorMap")
		bool bUseColorMap;

	UPROPERTY()
		TArray<FClientRepData> NetRelevancyLog;

	// Rate to poll for actor relevancy
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RenderTargetManager")
		float PollRelevancyTime;

	FTimerHandle NetRelevancyTimer_Handle;

	UPROPERTY()
		FBPVRReplicatedTextureStore RenderTargetStore;

	/*UFUNCTION()
		virtual void OnRep_TextureData()
	{
		DeCompressRenderTarget2D();
		// Write to buffer now
	}*/

	/*virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);

#if WITH_PUSH_MODEL
		FDoRepLifetimeParams Params;

		Params.bIsPushBased = true;
		DOREPLIFETIME_WITH_PARAMS_FAST(UVRRenderTargetManager, RenderTargetStore, Params);
#else
		DOREPLIFETIME(UVRRenderTargetManager, RenderTargetStore);
#endif
	}*/


	void InitRenderTarget()
	{
		UWorld* World = GetWorld();


		if (RenderTargetWidth > 0 && RenderTargetHeight > 0 && World)
		{
			//UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this, UCanvasRenderTarget2D::StaticClass(), RenderTargetWidth, RenderTargetHeight)

			RenderTarget = NewObject<UCanvasRenderTarget2D>(this);
			if (RenderTarget)
			{
				//NewCanvasRenderTarget->World = World;
				RenderTarget->InitAutoFormat(RenderTargetWidth, RenderTargetHeight);
				RenderTarget->ClearColor = ClearColor;
				RenderTarget->bAutoGenerateMips = false;
				RenderTarget->UpdateResourceImmediate(true);
			}
			else
			{
				RenderTarget = nullptr;
			}
		}
		else
		{
			RenderTarget =  nullptr;
		}
	}

	UFUNCTION(BlueprintCallable, Category = "VRRenderTargetManager|UtilityFunctions")
	bool GenerateTrisFromBoxPlaneIntersection(UPrimitiveComponent * PrimToBoxCheck, FTransform WorldTransformOfPlane, const FPlane & LocalProjectionPlane, FVector2D PlaneSize, FColor UVColor, TArray<FCanvasUVTri>& OutTris)
	{

		if (!PrimToBoxCheck)
			return false;

		OutTris.Reset();

		FBoxSphereBounds LocalBounds = PrimToBoxCheck->CalcLocalBounds();
		FVector Center = LocalBounds.Origin;
		FVector Extent = LocalBounds.BoxExtent;
		 
		// Transform into plane local space from our localspace
		FTransform LocalTrans = PrimToBoxCheck->GetComponentTransform() * WorldTransformOfPlane.Inverse();

		FVector BoxMin = Center - Extent;
		FVector BoxMax = Center + Extent;
		
		TArray<FVector> PointList;
		PointList.AddUninitialized(8); // 8 Is number of points on box

		PointList[0] = LocalTrans.TransformPosition(BoxMin);
		PointList[1] = LocalTrans.TransformPosition(BoxMax);
		PointList[2] = LocalTrans.TransformPosition(FVector(BoxMin.X, BoxMin.Y, BoxMax.Z));
		PointList[3] = LocalTrans.TransformPosition(FVector(BoxMin.X, BoxMax.Y, BoxMin.Z));
		PointList[4] = LocalTrans.TransformPosition(FVector(BoxMax.X, BoxMin.Y, BoxMin.Z));
		PointList[5] = LocalTrans.TransformPosition(FVector(BoxMin.X, BoxMax.Y, BoxMax.Z));
		PointList[6] = LocalTrans.TransformPosition(FVector(BoxMax.X, BoxMin.Y, BoxMax.Z));
		PointList[7] = LocalTrans.TransformPosition(FVector(BoxMax.X, BoxMax.Y, BoxMin.Z));

		// List of edges to check, 12 total, 2 points per edge
		int EdgeList[24] = 
		{ 
			0, 3, 
			0, 4,
			0, 2,
			2, 5,
			2, 6,
			4, 7,
			4, 6,
			6, 1,
			1, 7,
			1, 5,
			5, 3,
			3, 7
		};

		TArray<FVector2D> IntersectionPoints;

		FVector Intersection;
		float Time;

		FVector2D HalfPlane = PlaneSize / 2.f;
		FVector2D PtCenter;
		FVector2D NewPt;
		FVector PlanePoint;
		int CenterCount = 0;
		for(int i=0; i < 24; i+=2)
		{

			if (UKismetMathLibrary::LinePlaneIntersection(PointList[EdgeList[i]], PointList[EdgeList[i+1]], LocalProjectionPlane, Time, Intersection))
			{				
				//DrawDebugSphere(GetWorld(), WorldTransformOfPlane.TransformPosition(Intersection), 2.f, 32.f, FColor::Black);
				PlanePoint = Intersection;
				
				NewPt.X = ((PlanePoint.X + HalfPlane.X) / PlaneSize.X) * RenderTargetWidth;
				NewPt.Y = ((PlanePoint.Y + HalfPlane.Y) / PlaneSize.Y) * RenderTargetHeight;

				IntersectionPoints.Add(NewPt);
				PtCenter += NewPt;
				CenterCount++;
			}
		}

		if (IntersectionPoints.Num() <= 2)
		{
			return false;
		}

		// Get our center value
		PtCenter /= CenterCount;

		// Sort the points clockwise
		struct FPointSortCompare
		{
		public:
			FVector2D CenterPoint;
			FPointSortCompare(const FVector2D& InCenterPoint)
				: CenterPoint(InCenterPoint)
			{

			}

			FORCEINLINE bool operator()(const FVector2D& A, const FVector2D& B) const
			{
				if (A.Y - CenterPoint.X >= 0 && B.X - CenterPoint.X < 0)
					return true;
				if (A.X - CenterPoint.X < 0 && B.X - CenterPoint.X >= 0)
					return false;
				if (A.X - CenterPoint.X == 0 && B.X - CenterPoint.X == 0) {
					if (A.Y - CenterPoint.Y >= 0 || B.Y - CenterPoint.Y >= 0)
						return A.Y > B.Y;
					return B.Y > A.Y;
				}

				// compute the cross product of vectors (center -> a) x (center -> b)
				int det = (A.X - CenterPoint.X) * (B.Y - CenterPoint.Y) - (B.X - CenterPoint.X) * (A.Y - CenterPoint.Y);
				if (det < 0)
					return true;
				if (det > 0)
					return false;

				// points a and b are on the same line from the center
				// check which point is closer to the center
				int d1 = (A.X - CenterPoint.X) * (A.X - CenterPoint.X) + (A.Y - CenterPoint.Y) * (A.Y - CenterPoint.Y);
				int d2 = (B.X - CenterPoint.X) * (B.X - CenterPoint.X) + (B.Y - CenterPoint.Y) * (B.Y - CenterPoint.Y);
				return d1 > d2;
			}
		};

		IntersectionPoints.Sort(FPointSortCompare(PtCenter));

		FCanvasUVTri Tri;
		Tri.V0_Color = UVColor;
		Tri.V1_Color = UVColor;
		Tri.V2_Color = UVColor;

		OutTris.Reserve(IntersectionPoints.Num() - 2);

		// Now that we have our sorted list, we can generate a tri map from it, just doing a Fan from first to last
		for (int i = 1; i < IntersectionPoints.Num() - 1; i++)
		{
			Tri.V0_Pos = IntersectionPoints[0];
			Tri.V1_Pos = IntersectionPoints[i];
			Tri.V2_Pos = IntersectionPoints[i + 1];

			OutTris.Add(Tri);
		}

		return true;
	}

	UFUNCTION()
	void UpdateRelevancyMap()
	{
		AActor* myOwner = GetOwner();

		for (int i = NetRelevancyLog.Num() - 1; i >= 0; i--)
		{
			if (!NetRelevancyLog[i].PC.IsValid() || NetRelevancyLog[i].PC->IsLocalController() || !NetRelevancyLog[i].PC->GetPawn())
			{
				NetRelevancyLog[i].ReplicationProxy->Destroy();
				NetRelevancyLog.RemoveAt(i);
			}
			else
			{
				if (APawn* pawn = NetRelevancyLog[i].PC->GetPawn())
				{
					if (!myOwner->IsNetRelevantFor(NetRelevancyLog[i].PC.Get(), pawn, pawn->GetActorLocation()))
					{
						NetRelevancyLog[i].bIsRelevant = false;
						NetRelevancyLog[i].bIsDirty = false;
						//NetRelevancyLog.RemoveAt(i);
					}
				}
			}
		}


		bool bHadDirtyActors = false;

		for (FConstPlayerControllerIterator PCIt = GetWorld()->GetPlayerControllerIterator(); PCIt; ++PCIt)
		{
			if (APlayerController* PC = PCIt->Get())
			{
				if (PC->IsLocalController())
					continue;

				if (APawn* pawn = PC->GetPawn())
				{

					if (myOwner->IsNetRelevantFor(PC, pawn, pawn->GetActorLocation()))
					{
						FClientRepData * RepData = NetRelevancyLog.FindByPredicate([PC](const FClientRepData& Other)
							{
								return Other.PC == PC;
							});

						if (!RepData)
						{
							FClientRepData ClientRepData;
							
							FActorSpawnParameters SpawnParams;
							SpawnParams.Owner = PC;
							FTransform NewTransform = this->GetOwner()->GetActorTransform();
							ARenderTargetReplicationProxy* RenderProxy = GetWorld()->SpawnActor<ARenderTargetReplicationProxy>(ARenderTargetReplicationProxy::StaticClass(), NewTransform, SpawnParams);
							RenderProxy->OwningManager = this;
							RenderProxy->MaxBytesPerSecondRate = MaxBytesPerSecondRate;
							RenderProxy->TextureBlobSize = TextureBlobSize;

							if (RenderProxy)
							{
								RenderProxy->AttachToActor(this->GetOwner(), FAttachmentTransformRules::SnapToTargetIncludingScale);
							

								ClientRepData.PC = PC;
								ClientRepData.ReplicationProxy = RenderProxy;
								ClientRepData.bIsRelevant = true;
								ClientRepData.bIsDirty = true;
								bHadDirtyActors = true;
								NetRelevancyLog.Add(ClientRepData);
							}
							// Update this client with the new data
						}
						else
						{
							if (!RepData->bIsRelevant)
							{
								RepData->bIsRelevant = true;
								RepData->bIsDirty = true;
								bHadDirtyActors = true;
							}
						}
					}
				}

			}
		}

		if (bHadDirtyActors)
		{
			QueueImageStore();
		}
	}


	//UFUNCTION(BlueprintCallable, Category = "RenderTargetManager")
		bool DeCompressRenderTarget2D()
	{
		if (!RenderTarget)
			return false;

		RenderTargetStore.UnPackData();


		int32 Width = RenderTargetStore.Width;
		int32 Height = RenderTargetStore.Height;
		EPixelFormat PixelFormat = RenderTargetStore.PixelFormat;
		uint8 PixelFormat8 = 0;

		if (RenderTargetStore.bUseColorMap)
		{

		}
		else
		{
			TArray<FColor> FinalColorData;
			FinalColorData.AddUninitialized(RenderTargetStore.UnpackedData.Num());

			uint32 Counter = 0;
			FColor ColorVal;
			for (uint16 CompColor : RenderTargetStore.UnpackedData)
			{
				ColorVal.R = CompColor << 3;
				ColorVal.G = CompColor >> 5 << 2;
				ColorVal.B = CompColor >> 11 << 3;
				ColorVal.A = 0xFF;
				FinalColorData[Counter++] = ColorVal;
			}

			// Write this to a texture2d
			UTexture2D* RenderBase = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);// RenderTargetStore.PixelFormat);

			// Switched to a Memcpy instead of byte by byte transer
			uint8* MipData = (uint8*)RenderBase->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(MipData, (void*)FinalColorData.GetData(), FinalColorData.Num() * sizeof(FColor));
			RenderBase->PlatformData->Mips[0].BulkData.Unlock();

			//Setting some Parameters for the Texture and finally returning it
			RenderBase->PlatformData->SetNumSlices(1);
			RenderBase->NeverStream = true;
			RenderBase->SRGB = false;
			//Avatar->CompressionSettings = TC_EditorIcon;

			RenderBase->UpdateResource();

			/*uint32 size = sizeof(FColor);

			uint8* pData = new uint8[FinalColorData.Num() * sizeof(FColor)];
			FMemory::Memcpy(pData, (void*)FinalColorData.GetData(), FinalColorData.Num() * sizeof(FColor));

			UTexture2D* TexturePtr = RenderBase;
			const uint8* TextureData = pData;
			ENQUEUE_RENDER_COMMAND(VRRenderTargetManager_FillTexture)(
				[TexturePtr, TextureData](FRHICommandList& RHICmdList)
				{
					FUpdateTextureRegion2D region;
					region.SrcX = 0;
					region.SrcY = 0;
					region.DestX = 0;
					region.DestY = 0;
					region.Width = TexturePtr->GetSizeX();// TEX_WIDTH;
					region.Height = TexturePtr->GetSizeY();//TEX_HEIGHT;

					FTexture2DResource* resource = (FTexture2DResource*)TexturePtr->Resource;
					RHIUpdateTexture2D(resource->GetTexture2DRHI(), 0, region, region.Width * GPixelFormats[TexturePtr->GetPixelFormat()].BlockBytes, TextureData);
					delete[] TextureData;
				});*/

			FVector2D CanvasSize;
			FDrawToRenderTargetContext Context;

			// Using this as it saves custom implementation

			UWorld *World = GetWorld();

			// Reference to the Render Target resource
			FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();

			// Retrieve a UCanvas form the world to avoid creating a new one each time
			UCanvas* CanvasToUse = World->GetCanvasForDrawMaterialToRenderTarget();

			// Creates a new FCanvas for rendering
			FCanvas RenderCanvas(
				RenderTargetResource,
				nullptr,
				World,
				World->FeatureLevel);

			// Setup the canvas with the FCanvas reference
			CanvasToUse->Init(RenderTarget->SizeX, RenderTarget->SizeY, nullptr, &RenderCanvas);
			CanvasToUse->Update();

			if (CanvasToUse)
			{
				FTexture* RenderTextureResource = (RenderBase) ? RenderBase->Resource : GWhiteTexture;
				FCanvasTileItem TileItem(FVector2D(0, 0), RenderTextureResource, FVector2D(RenderTarget->SizeX, RenderTarget->SizeY), FVector2D(0, 0), FVector2D(1.f, 1.f), FLinearColor::White);
				TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Opaque);
				CanvasToUse->DrawItem(TileItem);


				// Perform the drawing
				RenderCanvas.Flush_GameThread();

				// Cleanup the FCanvas reference, to delete it
				CanvasToUse->Canvas = NULL;
			}

			RenderBase->ReleaseResource();
			RenderBase->MarkPendingKill();

		}

		return true;
	}


    UFUNCTION(BlueprintCallable, Category = "RenderTargetManager")
    void QueueImageStore() 
    {

        if (!RenderTarget || bIsStoringImage)
        {
            return;
        }

		bIsStoringImage = true;

		// Init new RenderRequest
		FRenderDataStore* renderData = new FRenderDataStore();

        // Get RenderContext
        FTextureRenderTargetResource* renderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();

		renderData->Size2D = renderTargetResource->GetSizeXY();
        renderData->PixelFormat = RenderTarget->GetFormat();

        struct FReadSurfaceContext {
            FRenderTarget* SrcRenderTarget;
            TArray<FColor>* OutData;
            FIntRect Rect;
            FReadSurfaceDataFlags Flags;
        };

        // Setup GPU command
        FReadSurfaceContext readSurfaceContext = 
        {
            renderTargetResource,
            &(renderData->ColorData),
            FIntRect(0,0,renderTargetResource->GetSizeXY().X, renderTargetResource->GetSizeXY().Y),
            FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX)
        };

		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[readSurfaceContext](FRHICommandListImmediate& RHICmdList) {
			RHICmdList.ReadSurfaceData(
				readSurfaceContext.SrcRenderTarget->GetRenderTargetTexture(),
				readSurfaceContext.Rect,
				*readSurfaceContext.OutData,
				readSurfaceContext.Flags
			);
		});

        // Notify new task in RenderQueue
        RenderDataQueue.Enqueue(renderData);

        // Set RenderCommandFence
        renderData->RenderFence.BeginFence();

        this->SetComponentTickEnabled(true);
    }

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
	{
		// Read pixels once RenderFence is completed
        if (RenderDataQueue.IsEmpty())
        {
            SetComponentTickEnabled(false);
        }
        else
		{
			// Peek the next RenderRequest from queue
			FRenderDataStore* nextRenderData;
            RenderDataQueue.Peek(nextRenderData);

            if (nextRenderData)
            {
                if (nextRenderData->RenderFence.IsFenceComplete())
                {				
					bIsStoringImage = false;
					RenderTargetStore.Reset();
					uint32 SizeOfData = nextRenderData->ColorData.Num();

					RenderTargetStore.UnpackedData.Reset(SizeOfData);
					RenderTargetStore.UnpackedData.AddUninitialized(SizeOfData);
					//FMemory::Memcpy(RenderTargetStore.UnpackedData.GetData(), (void*)nextRenderData->ColorData.GetData(), SizeOfData);

					uint16 ColorVal = 0;
					uint32 Counter = 0;
					
					// Convert to 16bit color
					for (FColor col : nextRenderData->ColorData)
					{
						ColorVal = (col.R >> 3) << 11 | (col.G >> 2) << 5 | (col.B >> 3);
						RenderTargetStore.UnpackedData[Counter++] = ColorVal;
					}

					FIntPoint Size2D = nextRenderData->Size2D;
					RenderTargetStore.Width = Size2D.X;
					RenderTargetStore.Height = Size2D.Y;
					RenderTargetStore.PixelFormat = nextRenderData->PixelFormat;
					RenderTargetStore.PackData();


#if WITH_PUSH_MODEL
					MARK_PROPERTY_DIRTY_FROM_NAME(UVRRenderTargetManager, RenderTargetStore, this);
#endif

					
                    // Delete the first element from RenderQueue
                    RenderDataQueue.Pop();
                    delete nextRenderData;

					for (int i = NetRelevancyLog.Num() - 1; i >= 0; i--)
					{
						if (NetRelevancyLog[i].bIsDirty && NetRelevancyLog[i].PC.IsValid() && !NetRelevancyLog[i].PC->IsLocalController())
						{
							if (NetRelevancyLog[i].ReplicationProxy.IsValid())
							{
								NetRelevancyLog[i].ReplicationProxy->TextureStore = RenderTargetStore;
								NetRelevancyLog[i].ReplicationProxy->SendInitMessage();
								//NetRelevancyLog[i].ReplicationProxy->ReceiveTexture(RenderTargetStore);
								NetRelevancyLog[i].bIsDirty = false;
							}
						}
					}


                }
            }
		}

    }

	virtual void BeginPlay() override
	{
		Super::BeginPlay();

		InitRenderTarget();

		if(bInitiallyReplicateTexture && GetNetMode() < ENetMode::NM_Client)
			GetWorld()->GetTimerManager().SetTimer(NetRelevancyTimer_Handle, this, &UVRRenderTargetManager::UpdateRelevancyMap, PollRelevancyTime, true);
	}

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
    {
        Super::EndPlay(EndPlayReason);

		FRenderDataStore* Store = nullptr;
		while (!RenderDataQueue.IsEmpty())
		{
			RenderDataQueue.Dequeue(Store);

			if (Store)
			{
				delete Store;
			}
		}

		if (GetNetMode() < ENetMode::NM_Client)
			GetWorld()->GetTimerManager().ClearTimer(NetRelevancyTimer_Handle);

		if (RenderTarget)
		{
			RenderTarget->ReleaseResource();
			RenderTarget = nullptr;
		}

		for (FClientRepData& RepData : NetRelevancyLog)
		{
			RepData.PC = nullptr;
			RepData.PC.Reset();
			if (RepData.ReplicationProxy.IsValid() && !RepData.ReplicationProxy->IsPendingKill())
			{
				RepData.ReplicationProxy->Destroy();
			}

			RepData.ReplicationProxy.Reset();
		}

    }

protected:
	TQueue<FRenderDataStore*> RenderDataQueue;

};


// Followed by a count of the following voxels
template <typename DataType>
void RLE_Funcs::RLEDecodeLine(TArray<uint8>* LineToDecode, TArray<DataType>* DecodedLine, bool bCompressed)
{
	if (!LineToDecode || !DecodedLine)
		return;

	RLEDecodeLine(LineToDecode->GetData(), LineToDecode->Num(), DecodedLine, bCompressed);
}

// Followed by a count of the following voxels
template <typename DataType>
void RLE_Funcs::RLEDecodeLine(const uint8* LineToDecode, uint32 Num, TArray<DataType>* DecodedLine, bool bCompressed)
{
	if (!bCompressed)
	{
		DecodedLine->Empty(Num / sizeof(DataType));
		DecodedLine->AddUninitialized(Num / sizeof(DataType));
		FMemory::Memcpy(DecodedLine->GetData(), LineToDecode, Num);
		return;
	}

	const uint8* StartLoc = LineToDecode;
	const uint8* EndLoc = StartLoc + Num;
	uint8 incr = sizeof(DataType);

	DataType ValToWrite = *((DataType*)LineToDecode); // This is just to prevent stupid compiler warnings without disabling them

	DecodedLine->Empty();

	uint8 RLE_FLAG;
	uint32 Length32;
	uint32 Length8;
	uint32 Length16;
	int origLoc;

	for (const uint8* loc = StartLoc; loc < EndLoc;)
	{
		RLE_FLAG = *loc >> 4; // Get the RLE flag from the first 4 bits of the first byte

		switch (RLE_FLAG)
		{
		case RLE_Flags::RLE_CompressedByte:
		{
			Length8 = (*loc & ~0xF0) + 1;
			loc++;
			ValToWrite = *((DataType*)loc);
			loc += incr;

			origLoc = DecodedLine->AddUninitialized(Length8);

			for (uint32 i = origLoc; i < origLoc + Length8; i++)
			{
				(*DecodedLine)[i] = ValToWrite;
			}

		}break;
		case RLE_Flags::RLE_CompressedShort:
		{
			Length16 = (((uint16)(*loc & ~0xF0)) << 8 | (*(loc + 1))) + 1;
			loc += 2;
			ValToWrite = *((DataType*)loc);
			loc += incr;

			origLoc = DecodedLine->AddUninitialized(Length16);

			for (uint32 i = origLoc; i < origLoc + Length16; i++)
			{
				(*DecodedLine)[i] = ValToWrite;
			}

		}break;
		case RLE_Flags::RLE_Compressed24:
		{
			Length32 = (((uint32)(*loc & ~0xF0)) << 16 | ((uint32)(*(loc + 1))) << 8 | (uint32)(*(loc + 2))) + 1;
			loc += 3;
			ValToWrite = *((DataType*)loc);
			loc += incr;

			origLoc = DecodedLine->AddUninitialized(Length32);

			for (uint32 i = origLoc; i < origLoc + Length32; i++)
			{
				(*DecodedLine)[i] = ValToWrite;
			}

		}break;

		case RLE_Flags::RLE_NotCompressedByte:
		{
			Length8 = (*loc & ~0xF0) + 1;
			loc++;

			origLoc = DecodedLine->AddUninitialized(Length8);

			for (uint32 i = origLoc; i < origLoc + Length8; i++)
			{
				(*DecodedLine)[i] = *((DataType*)loc);
				loc += incr;
			}

		}break;
		case RLE_Flags::RLE_NotCompressedShort:
		{
			Length16 = (((uint16)(*loc & ~0xF0)) << 8 | (*(loc + 1))) + 1;
			loc += 2;

			origLoc = DecodedLine->AddUninitialized(Length16);

			for (uint32 i = origLoc; i < origLoc + Length16; i++)
			{
				(*DecodedLine)[i] = *((DataType*)loc);
				loc += incr;
			}

		}break;
		case RLE_Flags::RLE_NotCompressed24:
		{
			Length32 = (((uint32)(*loc & ~0xF0)) << 16 | ((uint32)(*(loc + 1))) << 8 | ((uint32)(*(loc + 2)))) + 1;
			loc += 3;

			origLoc = DecodedLine->AddUninitialized(Length32);

			for (uint32 i = origLoc; i < origLoc + Length32; i++)
			{
				(*DecodedLine)[i] = *((DataType*)loc);
				loc += incr;
			}

		}break;

		case RLE_Flags::RLE_ContinueRunByte:
		{
			Length8 = (*loc & ~0xF0) + 1;
			loc++;

			origLoc = DecodedLine->AddUninitialized(Length8);

			for (uint32 i = origLoc; i < origLoc + Length8; i++)
			{
				(*DecodedLine)[i] = ValToWrite;
			}

		}break;
		case RLE_Flags::RLE_ContinueRunShort:
		{
			Length16 = (((uint16)(*loc & ~0xF0)) << 8 | (*(loc + 1))) + 1;
			loc += 2;

			origLoc = DecodedLine->AddUninitialized(Length16);

			for (uint32 i = origLoc; i < origLoc + Length16; i++)
			{
				(*DecodedLine)[i] = ValToWrite;
			}

		}break;
		case RLE_Flags::RLE_ContinueRun24:
		{
			Length32 = (((uint32)(*loc & ~0xF0)) << 16 | ((uint32)(*(loc + 1))) << 8 | (*(loc + 2))) + 1;
			loc += 3;

			origLoc = DecodedLine->AddUninitialized(Length32);

			for (uint32 i = origLoc; i < origLoc + Length32; i++)
			{
				(*DecodedLine)[i] = ValToWrite;
			}

		}break;

		}
	}
}

template <typename DataType>
bool RLE_Funcs::RLEEncodeLine(TArray<DataType>* LineToEncode, TArray<uint8>* EncodedLine)
{
	return RLEEncodeBuffer<DataType>(LineToEncode->GetData(), LineToEncode->Num(), EncodedLine);
}

void RLE_Funcs::RLEWriteContinueFlag(uint32 count, uint8** loc)
{
	if (count <= 16)
	{
		**loc = (((uint8)RLE_Flags::RLE_ContinueRunByte << 4) | ((uint8)count - 1));
		(*loc)++;
	}
	else if (count <= 4096)
	{
		uint16 val = ((((uint16)RLE_Flags::RLE_ContinueRunShort) << 12) | ((uint16)count - 1));
		**loc = val >> 8;
		(*loc)++;
		**loc = (uint8)val;
		(*loc)++;
	}
	else
	{
		uint32 val = ((((uint32)RLE_Flags::RLE_ContinueRun24) << 20) | ((uint32)count - 1));
		**loc = (uint8)(val >> 16);
		(*loc)++;
		**loc = (uint8)(val >> 8);
		(*loc)++;
		**loc = (uint8)val;
		(*loc)++;
	}
}

template <typename DataType>
void RLE_Funcs::RLEWriteRunFlag(uint32 count, uint8** loc, TArray<DataType>& Data, bool bCompressed)
{

	if (count <= 16)
	{
		uint8 val;
		if (bCompressed)
			val = ((((uint8)RLE_Flags::RLE_CompressedByte) << 4) | ((uint8)count - 1));
		else
			val = ((((uint8)RLE_Flags::RLE_NotCompressedByte) << 4) | ((uint8)count - 1));

		**loc = val;
		(*loc)++;
	}
	else if (count <= 4096)
	{
		uint16 val;
		if (bCompressed)
			val = ((((uint16)RLE_Flags::RLE_CompressedShort) << 12) | ((uint16)count - 1));
		else
			val = ((((uint16)RLE_Flags::RLE_NotCompressedShort) << 12) | ((uint16)count - 1));

		**loc = (uint8)(val >> 8);
		(*loc)++;
		**loc = (uint8)val;
		(*loc)++;
	}
	else
	{
		uint32 val;
		if (bCompressed)
			val = ((((uint32)RLE_Flags::RLE_Compressed24) << 20) | ((uint32)count - 1));
		else
			val = ((((uint32)RLE_Flags::RLE_NotCompressed24) << 20) | ((uint32)count - 1));

		**loc = (uint8)(val >> 16);
		(*loc)++;
		**loc = (uint8)(val >> 8);
		(*loc)++;
		**loc = (uint8)(val);
		(*loc)++;
	}

	FMemory::Memcpy(*loc, Data.GetData(), Data.Num() * sizeof(DataType));
	*loc += Data.Num() * sizeof(DataType);
	Data.Empty(256);
}

template <typename DataType>
bool RLE_Funcs::RLEEncodeBuffer(DataType* BufferToEncode, uint32 EncodeLength, TArray<uint8>* EncodedLine)
{
	uint32 OrigNum = EncodeLength;//LineToEncode->Num();
	uint8 incr = sizeof(DataType);
	uint32 MAX_COUNT = 1048576; // Max of 2.5 bytes as 0.5 bytes is used for control flags

	EncodedLine->Empty((OrigNum * sizeof(DataType)) + (OrigNum * (sizeof(short))));
	// Reserve enough memory to account for a perfectly bad situation (original size + 3 bytes per max array value)
	// Remove the remaining later with RemoveAt() and a count
	EncodedLine->AddUninitialized((OrigNum * sizeof(DataType)) + ((OrigNum / MAX_COUNT * 3)));

	DataType* First = BufferToEncode;// LineToEncode->GetData();
	DataType Last;
	uint32 RunCount = 0;

	uint8* loc = EncodedLine->GetData();
	//uint8 * countLoc = NULL;

	bool bInRun = false;
	bool bWroteStart = false;
	bool bContinueRun = false;

	TArray<DataType> TempBuffer;
	TempBuffer.Reserve(256);
	uint32 TempCount = 0;

	Last = *First;
	First++;

	for (uint32 i = 0; i < OrigNum - 1; i++, First++)
	{
		if (Last == *First)
		{
			if (bWroteStart && !bInRun)
			{
				RLE_Funcs::RLEWriteRunFlag(TempCount, &loc, TempBuffer, false);
				bWroteStart = false;
			}

			if (bInRun && /**countLoc*/TempCount < MAX_COUNT)
			{
				TempCount++;

				if (TempCount == MAX_COUNT)
				{
					// Write run byte
					if (bContinueRun)
					{
						RLE_Funcs::RLEWriteContinueFlag(TempCount, &loc);
					}
					else
						RLE_Funcs::RLEWriteRunFlag(TempCount, &loc, TempBuffer, true);

					bContinueRun = true;
					TempCount = 0;
				}
			}
			else
			{
				bInRun = true;
				bWroteStart = false;
				bContinueRun = false;

				TempBuffer.Add(Last);
				TempCount = 1;
			}

			// Begin Run Here
		}
		else if (bInRun)
		{
			bInRun = false;

			if (bContinueRun)
			{
				TempCount++;
				RLE_Funcs::RLEWriteContinueFlag(TempCount, &loc);
			}
			else
			{
				TempCount++;
				RLE_Funcs::RLEWriteRunFlag(TempCount, &loc, TempBuffer, true);
			}

			bContinueRun = false;
		}
		else
		{
			if (bWroteStart && TempCount/**countLoc*/ < MAX_COUNT)
			{
				TempCount++;
				TempBuffer.Add(Last);
			}
			else if (bWroteStart && TempCount == MAX_COUNT)
			{
				RLE_Funcs::RLEWriteRunFlag(TempCount, &loc, TempBuffer, false);

				bWroteStart = true;
				TempBuffer.Add(Last);
				TempCount = 1;
			}
			else
			{
				TempBuffer.Add(Last);
				TempCount = 1;
				//*countLoc = 1;

				bWroteStart = true;
			}
		}

		Last = *First;
	}

	// Finish last num
	if (bInRun)
	{
		if (TempCount <= MAX_COUNT)
		{
			if (TempCount == MAX_COUNT)
			{
				// Write run byte
				RLE_Funcs::RLEWriteRunFlag(TempCount, &loc, TempBuffer, true);
				bContinueRun = true;
			}

			if (bContinueRun)
			{
				TempCount++;
				RLE_Funcs::RLEWriteContinueFlag(TempCount, &loc);
			}
			else
			{
				TempCount++;
				RLE_Funcs::RLEWriteRunFlag(TempCount, &loc, TempBuffer, true);
			}
		}

		// Begin Run Here
	}
	else
	{
		if (bWroteStart && TempCount/**countLoc*/ <= MAX_COUNT)
		{
			if (TempCount/**countLoc*/ == MAX_COUNT)
			{
				// Write run byte
				RLE_Funcs::RLEWriteRunFlag(TempCount, &loc, TempBuffer, false);
				TempCount = 0;
			}

			TempCount++;
			TempBuffer.Add(Last);
			RLE_Funcs::RLEWriteRunFlag(TempCount, &loc, TempBuffer, false);
		}
	}

	// Resize the out array to fit compressed contents
	uint32 Wrote = loc - EncodedLine->GetData();
	EncodedLine->RemoveAt(Wrote, EncodedLine->Num() - Wrote, true);

	// If the compression performed worse than the original file size, throw the results array and use the original instead.
	// This will almost never happen with voxels but can so should be accounted for.

	return true;
	// Skipping non compressed for now, the overhead is so low that it isn't worth supporting since the last revision
	if (Wrote > OrigNum* incr)
	{
		EncodedLine->Empty(OrigNum * incr);
		EncodedLine->AddUninitialized(OrigNum * incr);
		FMemory::Memcpy(EncodedLine->GetData(), BufferToEncode/*LineToEncode->GetData()*/, OrigNum * incr);
		return false; // Return that there was no compression, so the decoder can receive it later
	}
	else
		return true;
}