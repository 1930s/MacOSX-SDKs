//
//  TKToken.h
//  Copyright (c) 2015 Apple. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#import <CryptoTokenKit/TKSmartCard.h>

NS_ASSUME_NONNULL_BEGIN

@protocol TKTokenSessionDelegate;
@class TKTokenSession;
@class TKTokenKeyAlgorithm;
@class TKTokenKeyExchangeParameters;
@protocol TKTokenDelegate;
@class TKToken;
@protocol TKTokenDriverDelegate;
@class TKTokenDriver;
@class TKTokenKeychainContents;
@class TKTokenAuthOperation;

/*!
 @abstract TKTokenObjectID Unique and persistent identification of objects on the token.
 @discussion Uniquely and persistently identifies objects (keys and certificates) present on the token.  Type of this identifier must be compatible with plist and its format is defined by the implementation of token extension.
 */
typedef id TKTokenObjectID
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE;

/*!
 @enum TKTokenOperation enumerates operations which can be performed with objects (keys and certificates) on the token.
 */
typedef NS_ENUM(NSInteger, TKTokenOperation) {
    TKTokenOperationNone                = 0,

    /*! Reading of raw data of certificate. */
    TKTokenOperationReadData            = 1,

    /*! Cryptographic signature using private key. */
    TKTokenOperationSignData            = 2,

    /*! Decrypting data using private key. */
    TKTokenOperationDecryptData         = 3,

    /*! Performing Diffie-Hellman style of cryptographic key exchange using private key. */
    TKTokenOperationPerformKeyExchange  = 4,
} __OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE;

/*!
 @abstract TKTokenOperationConstraint represents authentication constraint of token object for specific token operation.
 @discussion Persistently identifies constraint for performing specific operation on specific object.  Value of constraint can be either:
  - @YES: the operation is always allowed without any authentication needed
  - @NO: the operation is never allowed, typically not implemented
  - any other plist-compatible value: defined by the token extension implementation.  Such constraint is opaque to the system and is required to stay constant for the given object during the whole token's lifetime.  For example, smart card token extension might decide to use string 'PIN' to indicate that the operation is protected by presenting valid PIN to the card first.
 */
typedef id TKTokenOperationConstraint
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE;

/*!
 @abstract TKTokenKeyAlgorithm Encapsulates cryptographic algorithm, possibly with additional associated required algorithms.
 @discussion An algorithm supported by a key can be usually described by one value of @c SecKeyAlgorithm enumeration.  However, some tokens (notably smartcards) require that input data for the operation are in generic format, but that generic format must be formatted according to some more specific algorithm.  An example for this would be token accepting raw data for cryptographic signature but requiring that raw data are formatted according to PKCS1 padding rules.  To express such requirement, TKTokenKeyAlgorithm defines target algorithm (@c kSecKeyAlgorithmRSASignatureRaw in our example) and a set of other algorithms which were used (continuing example above, @c kSecKeyAlgorithmRSASignatureDigestPKCS1v15SHA1 will be reported as supported).
 */
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE
@interface TKTokenKeyAlgorithm : NSObject

- (instancetype)init NS_UNAVAILABLE;

/*!
 @brief Checks if specified algorithm is base operation algorithm.
 */
- (BOOL)isAlgorithm:(SecKeyAlgorithm)algorithm;

/*!
 @brief Checks whether specified algorithm is either target algorithm or one of the algorithms through which the operation passed.
 */
- (BOOL)supportsAlgorithm:(SecKeyAlgorithm)algorithm;

@end

/*!
 @abstract TKTokenKeyExchangeParameters Encapsulates parameters needed for performing specific Key Exchange operation types.
 */
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE
@interface TKTokenKeyExchangeParameters : NSObject

/*!
 @discussion Requested output size of key exchange result.  Should be ignored if output size is not configurable for specified key exchange algorithm.
 */
@property (readonly) NSInteger requestedSize;

/*!
 @discussion Additional shared information input, typically used for key derivation (KDF) step of key exchange algorithm.  Should be ignored if shared info is not used for specified key exchange algorithm.
 */
@property (readonly, nullable, copy) NSData *sharedInfo;

@end

/*!
 @abstract TKTokenSession represents token session which shares authentication status.
 @discussion Token implementation must inherit its own session implementation from TKTokenSession (or its subclass TKSmartCardTokenSession in case of smart card tokens).

 TKTokenSession should keep an authentication state of the token.  Authentication status (e.g. entered PIN to unlock smart card) should not be shared across borders of single TKTokenSession instance.

 TKTokenSession is always instantiated by TKToken when framework detects access to the token from new authentication session.
 */
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE
@interface TKTokenSession : NSObject

/*!
 @param token Token instance to which is this session instance bound.
 */
- (instancetype)initWithToken:(TKToken *)token NS_DESIGNATED_INITIALIZER;
@property (readonly) TKToken *token;
@property (weak, nullable) id<TKTokenSessionDelegate> delegate;

- (instancetype)init NS_UNAVAILABLE;

@end

/*!
 @abstract TKTokenSessionDelegate contains operations with token objects provided by token implementors which should be performed in the context of authentication session.
 */
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE
@protocol TKTokenSessionDelegate<NSObject>

@optional

/*!
 @discussion Establishes a context for the requested authentication operation.
 @param tokenSession Related TKTokenSession instance.
 @param operation Identifier of the operation.
 @param constraint Constraint to be satisfied by this authentication operation.
 @param error Error details (see TKError.h).
 @return authOperation Resulting context of the operation, which will be eventually finalized by receiving 'finishWithError:'.  The resulting 'authOperation' can be of any type based on TKTokenAuthOperation. For known types (e.g. TKTokenPasswordAuthOperation) the system will first fill in the context-specific properties (e.g. 'password') before triggering 'finishWithError:'. When no authentication is actually needed (typically because the session is already authenticated for requested constraint), return instance of TKTokenAuthOperation class instead of any specific subclass.
 */
- (nullable TKTokenAuthOperation *)tokenSession:(TKTokenSession *)session beginAuthForOperation:(TKTokenOperation)operation constraint:(TKTokenOperationConstraint)constraint error:(NSError **)error;

/*!
 @discussion Checks whether specified operation and algorithm is supported on specified key.
 @param tokenSession Related TKTokenSession instance.
 @param operation Type of cryptographic operation for which the list of supported algorithms should be retrieved.
 @param keyObjectID Identifier of the private key object.
 @param algorithm Algorithm with which the oepration should be performed.
 @return YES if the operation is supported, NO otherwise.
 */
- (BOOL)tokenSession:(TKTokenSession *)session supportsOperation:(TKTokenOperation)operation usingKey:(TKTokenObjectID)keyObjectID algorithm:(TKTokenKeyAlgorithm *)algorithm;

/*!
 @discussion Performs cryptographic signature operation.
 @param tokenSession Related TKTokenSession instance.
 @param dataToSign Input data for the signature operation.
 @param keyObjectID Identifier of the private key object.
 @param algorithm Requested signature algorithm to be used.
 @param error Error details (see TKError.h).  If authentication is required (by invoking beginAuthForOperation:), @c TKErrorCodeAuthenticationNeeded should be used.
 @return Resulting signature, or nil if an error happened.
 */
- (nullable NSData *)tokenSession:(TKTokenSession *)session signData:(NSData *)dataToSign usingKey:(TKTokenObjectID)keyObjectID algorithm:(TKTokenKeyAlgorithm *)algorithm error:(NSError **)error;

/*!
 @discussion Decrypts ciphertext using private key.
 @param tokenSession Related TKTokenSession instance.
 @param ciphertext Encrypted data to decrypt.
 @param keyObjectID Identifier of the private key object.
 @param algorithm Requested encryption/decryption algorithm to be used.
 @param error Error details (see TKError.h).  If authentication is required (by invoking beginAuthForOperation:), @c TKErrorCodeAuthenticationNeeded should be used.
 @return Resulting decrypted plaintext, or nil if an error happened.
 */
- (nullable NSData *)tokenSession:(TKTokenSession *)session decryptData:(NSData *)ciphertext usingKey:(TKTokenObjectID)keyObjectID algorithm:(TKTokenKeyAlgorithm *)algorithm error:(NSError **)error;

/*!
 @discussion Performs Diffie-Hellman style key exchange operation.
 @param tokenSession Related TKTokenSession instance.
 @param otherPartyPublicKeyData Raw public data of other party public key.
 @param keyObjectID Identifier of the private key object.
 @param algorithm Requested key exchange algorithm to be used.
 @param parameters Additional parameters for key exchange operation.  Chosen algorithm dictates meaning of parameters.
 @param error Error details (see TKError.h).  If authentication is required (by invoking beginAuthForOperation:), @c TKErrorCodeAuthenticationNeeded should be used.
 @return Result of key exchange operation, or nil if the operation failed.
 */
- (nullable NSData *)tokenSession:(TKTokenSession *)session performKeyExchangeWithPublicKey:(NSData *)otherPartyPublicKeyData usingKey:(TKTokenObjectID)objectID algorithm:(TKTokenKeyAlgorithm *)algorithm parameters:(TKTokenKeyExchangeParameters *)parameters error:(NSError **)error;

@end

/*!
 @discussion Class representing single token.  When implementing smart card based token, it is recommended to inherit the implementation from TKSmartCardToken.  Token object serves as synchronization point, all operations invoked upon token and all its sessions are serialized.
 */
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE
@interface TKToken : NSObject

/*!
 @discussion Initializes token instance
 @param tokenDriver Creating token driver.
 @param instanceID Unique, persistent identifier of this token.  This is typically implemented by some kind of serial number of the target hardware, for example smart card serial number.
 */
- (instancetype)initWithTokenDriver:(TKTokenDriver *)tokenDriver instanceID:(NSString *)instanceID NS_DESIGNATED_INITIALIZER;
@property (readonly) TKTokenDriver *tokenDriver;
@property (weak, nullable) id<TKTokenDelegate> delegate;

/*!
 @discussion Keychain contents (certificate and key items) representing this token.
 */
@property (nullable, readonly) TKTokenKeychainContents *keychainContents;

- (instancetype)init NS_UNAVAILABLE;

@end

/*!
 @abstract TKTokenDelegate contains operations implementing functionality of token class.
 @discussion TKTokenDelegate represents protocol which must be implemented by token implementors' class representing token.  Apart from being able to identify itself with its unique identifier, and must be able to establish new TKTokenSession when requested.
 */
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE
@protocol TKTokenDelegate<NSObject>

@required

/*!
 @abstract Create new session instance
 @discussion All operations with objects on the token are performed inside TKTokenSession which represent authentication context.  This method is called whenever new authentication context is needed (typically when client application wants to perform token operation using keychain object which has associated LocalAuthentication LAContext which was not yet seen by this token instance).
 @param token Related token instance.
 */
- (nullable TKTokenSession *)token:(TKToken *)token createSessionWithError:(NSError **)error;

@optional

/*!
 @abstract Terminates previously created session, implementation should free all associated resources.
 @param token Related token instance.
 */
- (void)token:(TKToken *)token terminateSession:(TKTokenSession *)session;

@end

/*!
 @discussion Base class for token drivers.  Smart card token drivers should use TKSmartCardTokenDriver subclass.
 */
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE
@interface TKTokenDriver : NSObject

@property (weak, nullable) id<TKTokenDriverDelegate> delegate;

@end

/*!
 @discussion Delegate for customizing token driver operations.  Smart card tokens should implement TKSmartcardTokenDriverDelegate instead of this base protocol.
 */
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE
@protocol TKTokenDriverDelegate<NSObject>

@optional

/*!
 @discussion Terminates previously created token, should release all resources associated with it.
 */
- (void)tokenDriver:(TKTokenDriver *)driver terminateToken:(TKToken *)token;

@end

/*!
 @discussion Context of a pending authentication operation.
 */
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE
@interface TKTokenAuthOperation : NSObject<NSSecureCoding>

/*!
 @discussion Handler triggered by the system in order to let the token finalize the authentication operation.
 @param error Error details (see TKError.h).
 @return Finalization status.
 */
- (BOOL)finishWithError:(NSError **)error;

@end

/*!
 @discussion Context of a password authentication operation.
 */
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0) __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE
@interface TKTokenPasswordAuthOperation : TKTokenAuthOperation

/*!
 @discussion Password, which will be filled in by the system when 'finishWithError:' is called.
 */
@property (nullable, copy) NSString *password;

@end

NS_ASSUME_NONNULL_END
