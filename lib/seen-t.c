/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2011 Richard Kettlewell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */
#include <config.h>

#include "seen.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int debug;

static const char *commits[] = {
    "d03c87db50e7679b24f69088cafa4e17b7dc9cdf",
    "9764a203e172e10ae593423af0db7e8022ea1f15",
    "ab71e0e422d8bd26a3ac3bb8064b4564ca7efc3e",
    "3cb990b6dd1a4790e740c7666211b580d3314165",
    "35e53f532eddc5bcc94390cf47f44e3a6c9be275",
    "02a54035a6f053f075ae049919e3491d6b7474f9",
    "9aa9b48f44738c1d76fa04f3105db1b970df45b5",
    "9a2fc5a3508d5b153db417cbe67f185afed8be5a",
    "c38a8108a1edd6968788facb1d903b15b7879bd1",
    "8aea7659a9577ceef8f74a1de8c3fccdc8f09ec2",
    "be60ae999ae74f2f9d6a3378862b756584d1eaf6",
    "436b1beb8bb7b1eb31c80847834f9dfc38c3ed65",
    "10e3d706876b670e41ea2d637d06d897b09a2409",
    "72f3a7b78ae61ca3587b87f5b7acb89047882ea5",
    "fb6ec038e71fc288abc56a3928ae38b73f6b039b",
    "b4b8871483dcc8eef78fb8577687fedb5183bad6",
    "be34591aa424e3da075e1347080bea06c239a7c0",
    "2af3a8ee4e8edce1d765d8007d8cc82aabe346c1",
    "f68d66a299b0b3f852aa1764517e7f8da7fd3619",
    "a386e6a0b6c3bb70e22649375828b416b37646cb",
    "d3e9527c36f9200ed4ef9c6bb4b9ca1f892067cf",
    "fccee6643a93650ee90f42b156dc01f0c7530875",
    "b5c51ccd4057a55820934efa9bbaad2e72e0bd45",
    "394b3c1f44b1f7ddafa6ab91a97efd705053de6b",
    "1f8f5d92a25a9fb78fd0e29ba3b2245181110089",
    "018a64d375a6348664424dc02a806ec56ad2871a",
    "99cf9c1242ea86cd2b0f140500ba80851cf959f9",
    "bb3bd13e3462b2276f1a3d1da6c39f52a31a17f8",
    "85cdc0410fc48ec10d8bb53a603a0e708108808d",
    "0bc1280e48e5ae8bb9106f031d7299a196597aba",
    "f45d1b9a75976cb2b7859442548fb3debc7ce798",
    "0e499065185889bbffd04793a2fedaef1f27c9e6",
    "e8abaea137cd507b255efd75ade48c5647824930",
    "5f6cb18720910dea3fa6fbc4332f22a697ec219a",
    "de189d7f438b236269e8e8751157fd09af25ebe1",
    "03fca3b8e5ec0c61afd8ff59daa74dd5337a4b84",
    "27bc0a33adf1da4236dd5c2f04baf7c163b42e52",
    "98e98e0db5982f536e94f271af9c3a1d795dc5f8",
    "88f4eab91486576a17778f26f23ef8c09bbf46d8",
    "fb71f5bbcb4f29336de434b2a4f42d39a353ca47",
    "2a8c5ffd52b45766cb976ed1fd30241e440fcef2",
    "682974d1b4f82eade3db52b3959d31ea7b5df8bc",
    "7e5e11b3680e770adc0ccd4d339b495b0fb748a0",
    "51bbc99d1e87544323397f076ef5a4f57ebc15a9",
    "b49a340a5132f0fd1545e2c8b52c698393ab5b9e",
    "0109913243e0a079272d0420b3a848978751ae99",
    "766df7853960ccdd942d21e8c0b0a191460fa41c",
    "520bc8ece27661aef52c0ad0f34cbe403401a7ae",
    "c67d69aa9d619d7e5dc81df7cea548dfff47b9e9",
    "6c62d12e9696923309375c67561a1ed5d4ee69e3",
    "3dcae437c5157288c305f35e68eb1dd0d92b0f4e",
    "82084cd1f9d1f751b353faf06b521f7a29def8d9",
    "0d27bc5928d30422a928eb5139e5d0019baa94ba",
    "10e1bde418215d52117b3192d5b15fd122405648",
    "7a40b37cb88fcf46f1a6715f21985651148332e6",
    "16860d700d8a3fd55d77b29bb0b3327593ebf866",
    "2a44f2543e24a2194c1007f4f96c4ef792d14af7",
    "e27957a6ab765fbcf87a6d5c6423a352d224ff0e",
    "e0802cab3e83950b3e359be139bc0b427ba5576a",
    "35931046956625cc9addd6d11d1c5b3dfdb71a9c",
    "fc6e5f3dde3b7ffc6814180605dd4cd36b93fbaa",
    "ba872054afa0695bef7374f41aa8106b350789eb",
    "53fb2f7dbf0ab7083f59ffdba81a07cb6435bfa5",
    "b6bfab35c9b4f1deebe4de5057ca650fcebba42d",
    "d9cb0b7dbb3af5aceb2767059e0b485bb364fbfb",
    "355cfc03e66665c83b82c579c32e455a37edbb70",
    "cd1b9fbaecc5cec4d418d1f4558af6a398656a84",
    "16f96b94165452b5d8b2ae5674f0a18a1a425b4a",
    "3168ef1457612008b67c0326324f8d4e9d00fb85",
    "9f6d5fa877a418246e32ecc80b977aed64a6ac68",
    "c4d768cf0091cc663636e494f7bd34a1afa44995",
    "e44e34a848e2ad7667bd60b5817c9e468a26ab73",
    "f31396372cfed2217c68364cbb2a30bae3c2b114",
    "44dc7347bbaf3de3692d95334b882eb44a457ecd",
    "011cc11a5326397d472826ea73bd529bda5e047c",
    "e6a732ef1942e5dc55c4a47e8624b51187f50101",
    "9c8676591aa1e3797e3630cfbece3d1f81fbdbd2",
    "329e4ebf171e12ddd2a02b3ca62b8ccf6cdb31ad",
    "8adb670e3cb999c0983973248f14b9d3de860d50",
    "c640211d80538f95b7aece27bbd779f10bb88bcf",
    "1996b80cc10468b75aaaeb82cf8ba07ccc0f3c84",
    "1f6df36fb77fdc97d023b1a58980926dac752841",
    "86f2b98c39489d07fe689eacdc4fdf9c02c57eb4",
    "6872d5b3b5bfbfb52afbd7ef2d45adb64fbfde08",
    "1e926fcb1dc187cd2c26b90f36052fc48ea9e41b",
    "7dfc8e464d20a32db957d5110f29fceac2ce587c",
    "04f0ab8380c723a16340cb44c85bda0dca7f12fe",
    "a2a65f427c66ed457dd78f268641e43ece3115f5",
    "b44247a5e9f5cfb9bf76f378dc0d85e746956e2b",
    "d7d46e87767774c35235221866d92407c8145940",
    "cb654ad04ccb5d98afe8b33e9a3f994cc891ad74",
    "078ded8fc3e67e9a8c2619b4a2d489baf90b5800",
    "90e0ce4ed44652734e618283599b94db1313a91c",
    "c6c6ad86e5b65c4925ec203029a3c794318ad3f8",
    "faacb4b7cdbc451780fa90512d946f26a4832f44",
    "abec5b8957f2c70ee94c6f55971d9715d5688e16",
    "0034621551210e527ba2e29c6a805a90e466d2eb",
    "eb474557516ecf21f8a1c1984222bdd44445b735",
    "51f74ebeffe2d07f5c27783af18fa0e10e3bbc89",
    "12e73619071515f44867ba218f6b4ec0ef828ef3",
    "8f3e8d69db9b6a50665f847d35e391c9d117bb11",
    "f9b004672e0520dc29863f343b50ec459a94db09",
    "74a3507024996764cb553ff6dd01abf0e2884b1a",
    "3f9dc9018a3a23401e5d97a20aac49d38f2c40f1",
    "b8e4d74ea139ffdbb4df3dd1b5b158b37c6d55a2",
    "58957ca6d5a4caa41085901f166a7298fe594bdf",
    "65cf53d59732d13db40ba5efe927122ab0a440ef",
    "02f08fe9b8d140f95d3e259d4ff4bc3c4b6ea818",
    "c84b6f4681c5cd6f9f4eb2247041b06eb483395f",
    "8c9e27a84ed5a30b1898e5a6fd6782e3298147ff",
    "d3c5a498467c5e939a23db883881cc754063d3c7",
    "32626df5b45c8ee97a9fb99dc0f474c7aff279ec",
    "1ca2155dbccee7134e830ddb473f1cd0e1e068fa",
    "604cd8420bdf80212ff7dcf5aa07e690e1e40f39",
    "d600ee6b8ec507a66f62bed2d1b968f0f63be5a3",
    "b2cafbbb1a341205c09978b655eb3247a97b8517",
    "2635a4025ce2d424658c2ea5bc536c9d4f8fd4d6",
    "5523114e9294205b738391ed0ede2272193c3c25",
    "b52128372cde11ef557e27652dd4546e45a871cc",
    "e6afe45ed18281b4ac92aa416e0bfc472505c009",
    "722571c8179465cfc9abd34953ff32eb95afaa4c",
    "9de0bccc1c0155c8abc0298b6aa276fbe3568c8e",
    "a3ea87cc5f1827b010cb925f441456c67a12437d",
    "debc4278f94b60bc69843ee38fad2c29a4630bad",
    "102a5bda4e7e3ef58d674e64e70269e0f22416ea",
    "0c0dca8eb153c04bd0fec0e8d4c032bcfa452cd2",
    "4923c872d3d0d4c6b1a7be66a580db5fc5b24f21",
    "3cdbf4cf25cd33518430a12f6d515d4eb534a079",
    "32e399a6af7123556dd100c9cbc60d289caa7d9b",
    "29188294b4d64d9e6f3836cef3dea42f87e0b292",
    "0662fe5dcda0c09747ec74a9a1c7dc6288c6eddb",
    "2aa0a0e22757ada42b3cc65e788b276b899403f8",
    "dc5d5ef55576cbb7e1265ff7edb2c0020e773693",
    "1eecb467266af405dfe9482011b2f91162e6f755",
    "19b8a38010a7e044243059da4555276508f701f0",
    "5296296a9ed87d7f0a7233b2282acbaf1c34facd",
    "389d72d4b283922ebe59b2529b565bc1b2506895",
    "d97e63f18a4770a769e1074b1de4869f7ef4d2d1",
    "aa1d439bd6e5b02eb667a295ff5104261b813efd",
    "684f78cc24902f64ede12c00867f019c89bea5ed",
    "917320c9626f1c729f7c0fd70b05d88952c27553",
    "512102349f81bb8647d4e4c0777d20a587d0a04e",
    "6a8106ffd77e22a998a36aef4847fb150deb1f5d",
    "fdeab60aba96a50a7f9f9e13e6bac8092a4676d9",
    "82191db0b8765f58e0857f7bcd0b2fc6dfc66acf",
    "67632747746015941863799939251f2651613722",
    "1f3d0d1efbfc4c9f514fe60490e06eb59780b3b7",
    "9b53bbf06242d4d8605bcfe9b5a640c5d5d75c41",
    "b05afe2d8accd99fbcef72019e1fd1dcc580ee35",
    "7e3b93a9bdec5f8af22bedfed2f9d43c9d6a3244",
    "7fd92a65747102f1bc6bccef9971c06792a26e99",
    "2edf555c593d96e0392357d192db9322d489e901",
    "95c9a5d5e60fc765917447889aacd8acfed9df7c",
    "bf46676811655c175e0cc8b3c14458d537d4f5da",
    "8976974adfe8cb132f4f2950270e767b38a1e34b",
    "1b3eaa797635684d99244aae224e8d509a474efb",
    "3eeda9ebd0428ad8bfc3b47eac6d6176602f5391",
    "cfc28774ba654161a01d64ca215b5a3364a3d588",
    "35657fbdb89faea7bffdc8adf72fcba286634bcb",
    "7ba7a0ae5c418a27c73fdbf7c2656640919e701b",
    "5857b22472a5e085c04ca59e5d7ed2e8096ce20d",
    "2780f3d552d874a23c292a0b31b23fbdf4f1b649",
    "cd1470c904edb9a085a487d4b7ba5e387351e59f",
    "7efe299a42038ef2a57715491ab5a25f71eda7a0",
    "fc35d300306924dec24a942b602f8a63e0193c54",
    "2e61f6dc3cd4e6a79b647a152dae166f86996aed",
    "667c74d6a1d9ef45cb531393919454b909631c1f",
    "4b8afd0e52d57cabcd4fc93cf02e79ddab0d0105",
};

#define NCOMMITS (sizeof commits / sizeof *commits)

int main() {
  size_t n;
  debug = !!getenv("DEBUG");
  remove("s1.seen");
  init_seen("s1.seen");
  for(n = 0; n < NCOMMITS; ++n)
    assert(!seen(commits[n]));
  for(n = 0; n < NCOMMITS; ++n)
    remember(commits[n]);
  for(n = 0; n < NCOMMITS; ++n)
    assert(seen(commits[n]));
  init_seen("s2.seen");
  for(n = 0; n < NCOMMITS; ++n)
    assert(!seen(commits[n]));
  init_seen("s1.seen");
  for(n = 0; n < NCOMMITS; ++n)
    assert(seen(commits[n]));
  remove("s1.seen");
  remove("s2.seen");
  return 0;
}
